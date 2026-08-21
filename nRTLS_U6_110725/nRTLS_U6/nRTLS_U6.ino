

/** CHƯƠNG TRÌNH XỬ LÝ DECAWAVE CHO BASE 03
   1. nhận tag
   2. nhận sync_rx không dây
   3. sync_rx wire
   4. setup thông số qua spi
   5. gửi thông số tag lên server qua spi
   6. led status được điều khiển qua spi
   7. phát beacon
   8. tinh twr
 * **/

#include <SPIFFS.h>
// #include <WiFiClient.h>
#include <EEPROM.h>
#include <TimeOutEvent.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>

#include "DataBase.h"
#include "Handle_FTP.h"
#include "OTA.h"
#include "define.h"
#include "handle_ethernet.h"
#include "handle_logfile.h"
#include "handle_mqtt.h"
#include "handle_sd_card.h"
#include "handle_sdcard.h"
#include "handle_spi_master.h"
#include "handle_tcp.h"
#include "handle_wifi.h"
#include "web_server.h"

// #include "iBeaconTest.h"
#include "handle_com_regs.h"
#include "mac_hostname_u6.h" // Thang add

#include "button_control.h"     // PTB add
#include "reset_factory_dhcp.h" // PTB add
#include "uart_config.h"        // PTB add

extern void sendmqtt_request_data(request_data_t *req);

/*
  {"Serial":"TEST","MessageID":2,"CMD":2,"FileName":"nRTLS3_D_H1.00_F1.01.bin","HwVer":"1.0","FwVer":"2.0","Path":"/BaseStation","MD5":"F93D282DC2056D38BF546F702CDD6A3E","Reply":""}
  {"SerialID":120116,"MessageID":2,"CMD":8,"BaseID":48628,"Distance":0,"Reply":""}
  {"Serial":250122,"MessageID":2,"CMD":11,"Access":1,"isEnable":true,"Serial_Master":250122,"Timestamp":0,"Reply":""}
*/

volatile uint8_t counteroff = 0;
volatile bool flag = false;
com_frame_t tx_frame;
TimeOutEvent ota_TimeCheck(3000);
// TimeOutEvent fragStatusTimer(0);
TimeOutEvent u7_TimeCheck(300000); // 5 phút check 1 lần xem u7 có treo không

uint8_t debug_TDOA;
uint8_t compare_flag;

// ISP3080 version (nhận 1 lần từ U7, gửi MQTT mỗi 20s)
char g_isp_fw_version[10] = "0.00";
char g_isp_hw_version[10] = "0.00";
bool g_isp_version_received = false;

// Chỉ gửi lại CMD20 sau khi U7/ISP đã ổn định 10 giây kể từ lúc trả version.
static bool g_bcast_twr_restore_pending = false;
static unsigned long g_bcast_twr_restore_at = 0;

// Callback SPI chạy ở task/core xử lý U7: không được gọi WiFiClient trực tiếp tại đây.
// Queue giới hạn giúp lưu lượng BSS-TWR không làm nghẽn mqtt_loop() trên Internet Core 0.
typedef struct {
  uint16_t len;
  uint8_t data[128];
} bcast_twr_net_frame_t;

static QueueHandle_t g_bcast_twr_net_queue = NULL;
static volatile uint32_t g_bcast_twr_queue_dropped = 0;

volatile bool g_ota_ack_enter_received = false;
volatile bool g_ota_ack_enter_status = false;
volatile bool g_ota_ack_data_received = false;
volatile bool g_ota_ack_data_status = false;
volatile bool g_ota_ack_end_received = false;
volatile bool g_ota_ack_end_status = false;

typedef struct {
  uint32_t fw_size;
  uint32_t crc32_val;
} isp_ota_enter_t;

typedef struct {
  uint32_t offset;
  uint16_t len;
  uint8_t data[240];
} isp_ota_data_t;

void Taks_Spi_rx(uint8_t *data, uint16_t length) {
  if (debug_TDOA == 1) {
    for (unsigned int i = 0; i < length; i++) {
      debug_SPI("%c", (char)data[i]);
    }
  }

  // Phân luồng SPI data theo prefix:
  // "D," -> DS-TWR distance result -> MQTT publish
  // "T," -> BSS-TWR data -> TCP port 2011
  // Khác  -> TDOA data -> TCP port cũ

  if (length >= 2 && data[0] == 'D' && data[1] == ',') {
    // DS-TWR result: "D,<deviceID>,<distance>:\r\n"
    // Parse deviceID và distance rồi publish MQTT
    unsigned long dev_id = 0, dist = 0;
    if (sscanf((char *)data, "D,%lu,%lu:", &dev_id, &dist) == 2) {
      if (MQTT_Exchange.CMD == 20) {
        sendmqtt_distance_cmd10(Config_Device.Device.SerialID, (uint32_t)dist);
      } else {
        sendmqtt_distance((uint32_t)dev_id, (uint32_t)dist);
      }
      dbg_main("SPI TWR: devID=%lu dist=%lu -> MQTT", dev_id, dist);
    }
  } else if (length >= 2 && data[0] == 'T' && data[1] == ',') {
    // BSS-TWR data: chỉ xếp queue tại callback SPI; task Internet sẽ gửi TCP 2011.
    bcast_twr_net_frame_t frame;
    int formatted_len = snprintf((char *)frame.data, sizeof(frame.data),
                                 "T,%lu,%.*s",
                                 (unsigned long)Config_Device.Device.SerialID,
                                 length - 2, data + 2);
    if (formatted_len > 0 && g_bcast_twr_net_queue != NULL) {
      frame.len = (uint16_t)min(formatted_len, (int)sizeof(frame.data) - 1);
      if (xQueueSend(g_bcast_twr_net_queue, &frame, 0) != pdPASS) {
        // Không block SPI khi TCP chậm/mất kết nối. Bỏ frame cũ để giữ dữ liệu mới.
        bcast_twr_net_frame_t old_frame;
        xQueueReceive(g_bcast_twr_net_queue, &old_frame, 0);
        if (xQueueSend(g_bcast_twr_net_queue, &frame, 0) != pdPASS) {
          g_bcast_twr_queue_dropped++;
        }
      }
    }
  } else {
    // TDOA/raw data. Khi đang BSS-TWR chỉ cho phép frame "T," đi port 2011;
    // các frame dạng "0,...:" / "1,...:" là TDOA cũ, phải drop để tránh lọt lên TCP.
    if (g_base_bcast_twr_active) {
      dbg_main("BSS-TWR active: drop non-T frame prefix=%c len=%d", data[0], length);
      return;
    }
    _handle_tcp.sendTCP((uint8_t *)data, length);
  }

  debug_TCP("send tcp: %d\n", length);
}

void Taks_Uart_rx(uint8_t *data, uint16_t length) {
  com_frame_t rx_frame;
  memcpy((uint8_t *)&rx_frame, data, length);
  uint16_t calcCRC =
      SPI_master.calcCRC(rx_frame.data, rx_frame.header.msk_regs.len);

  dbg_main("UartRX: len=%d, type=%02X, add=%04X, msk_len=%d, rxCRC=%04X, "
           "calcCRC=%04X",
           length, rx_frame.header.type, rx_frame.header.msk_regs.add,
           rx_frame.header.msk_regs.len, rx_frame.header.check_crc, calcCRC);

  // TWR Result qua UART đã deprecated — dùng SPI → TCP port 2011

  if (calcCRC == 0 && rx_frame.header.type == 0) {
    dbg_main("UART ERROR");
  } else if (calcCRC == rx_frame.header.check_crc &&
             rx_frame.header.type <= read_twr_result) {
    dbg_main("UART OK: CRC Match! target add=%04X",
             rx_frame.header.msk_regs.add);

    if (rx_frame.header.msk_regs.add == SET_TWO_WAY.add) {
      memcpy(&two_way, rx_frame.data, rx_frame.header.msk_regs.len);
      dbg_main("deviceID: %lu", two_way.deviceID);
      dbg_main("distance: %lu", two_way.distance);
      if (MQTT_Exchange.CMD == 20) {
        // Measure SS_TWR Base-Tag (triggered by CMD 20)
        sendmqtt_distance_cmd10(Config_Device.Device.SerialID,
                                two_way.distance);
      } else {
        // Measure TWR Base-Base (CMD 8)
        sendmqtt_distance(two_way.deviceID, two_way.distance);
      }
    }
    // TWR_RESULT_RES qua UART đã deprecated — dùng SPI → TCP port 2011
    else if (rx_frame.header.msk_regs.add == SerialID_RES.add) {
      memcpy(&SerialID, rx_frame.data, rx_frame.header.msk_regs.len);
      dbg_main("SerialID: %lu", SerialID.device);
      // Config_Device.Device.SerialID = SerialID.device;//Hieu addeed
      Fag_mask_regs.FAG_SerialID_RES = 0;
      dbg_main("read add %X\n\r", rx_frame.header.msk_regs.add);
    } else if (rx_frame.header.msk_regs.add == DW_CONFIG_RES.add) {
      memcpy(&dw_config_twr, rx_frame.data, rx_frame.header.msk_regs.len);
      Fag_mask_regs.FAG_DW_CONFIG_RES = 0;
      dbg_main("read config DW success \n\r");
    }
    // ==== [NEW] ISP3080 OTA BINARY ACK ====
    else if (rx_frame.header.msk_regs.add == 0x10A0) {
      g_ota_ack_enter_status = (rx_frame.data[0] == 1);
      g_ota_ack_enter_received = true;
    } else if (rx_frame.header.msk_regs.add == 0x10A1) {
      g_ota_ack_data_status = (rx_frame.data[0] == 1);
      g_ota_ack_data_received = true;
    } else if (rx_frame.header.msk_regs.add == 0x10A2) {
      g_ota_ack_end_status = (rx_frame.data[0] == 1);
      g_ota_ack_end_received = true;
    } else if (rx_frame.header.msk_regs.add == 0x10A3) {
      // ABORT ACK
    } else if (rx_frame.header.msk_regs.add == DW_CONFIG_TX_RES.add) {
      memcpy(&dw_txconfig_twr, rx_frame.data, rx_frame.header.msk_regs.len);
      Fag_mask_regs.FAG_DW_CONFIG_TX_RES = 0;
      dbg_main("read add %X\n\r", rx_frame.header.msk_regs.add);
    } else if (rx_frame.header.msk_regs.add == DW_ANT_DELAY_RES.add) {
      memcpy(&anten_delay_twr, rx_frame.data, rx_frame.header.msk_regs.len);
      Fag_mask_regs.FAG_DW_ANT_DELAY_RES = 0;
      dbg_main("read add %X\n\r", rx_frame.header.msk_regs.add);
    } else if (rx_frame.header.msk_regs.add == MASTER_RES.add) {
      memcpy(&dw_master_twr, rx_frame.data, rx_frame.header.msk_regs.len);
      Fag_mask_regs.FAG_MASTER_RES = 0;
      dbg_main("read add %X\n\r", rx_frame.header.msk_regs.add);
    } else if (rx_frame.header.msk_regs.add == MASTER_ACCESS_RES1.add) {
      dbg_main("access1");
      Fag_mask_regs.FAG_MASTER_ACCESS_RES1 = 0;
      memcpy(&master_access, rx_frame.data, rx_frame.header.msk_regs.len);
      repost_accept(1);
    }

    else if (rx_frame.header.msk_regs.add == MASTER_ACCESS_RES2.add) {
      dbg_main("access2");
      Fag_mask_regs.FAG_MASTER_ACCESS_RES2 = 0;
      memcpy(&master_access, rx_frame.data, rx_frame.header.msk_regs.len);
      repost_accept(2);
    }

    else if (rx_frame.header.msk_regs.add == MASTER_ACCESS_RES3.add) {
      dbg_main("access3");
      Fag_mask_regs.FAG_MASTER_ACCESS_RES3 = 0;
      memcpy(&master_access, rx_frame.data, rx_frame.header.msk_regs.len);
      repost_accept(3);
    }

    else if (rx_frame.header.msk_regs.add == MASTER_ACCESS_RES4.add) {
      dbg_main("access4");
      Fag_mask_regs.FAG_MASTER_ACCESS_RES4 = 0;
      memcpy(&master_access, rx_frame.data, rx_frame.header.msk_regs.len);
      repost_accept(4);
    }

    else if (rx_frame.header.msk_regs.add == READ_VERSION.add) {
      Fag_mask_regs.FAG_VERSION_RES = 0;
      uint8_t dataVersion[rx_frame.header.msk_regs.len + 1];
      memcpy(&dataVersion, rx_frame.data, rx_frame.header.msk_regs.len);
      // dbg_main("version data:%s",(char*)dataVersion);
      char *p;
      p = strtok((char *)dataVersion, "-");
      sprintf(&Config_Device.Version_decawave.FW_Ver[0], "%s", p);
      if (p != NULL) {
        p = strtok(NULL, "-");
        sprintf(&Config_Device.Version_decawave.HW_Ver[0], "%s", p);
      }
      dbg_main("FW:%s", Config_Device.Version_decawave.FW_Ver);
      dbg_main("HW:%s", Config_Device.Version_decawave.HW_Ver);
      dbg_main("read add %X\n\r", rx_frame.header.msk_regs.add);
      // delay(1000);
      read_button_state();
    }

    else if (rx_frame.header.msk_regs.add == ISP_VERSION_RES.add) {
      // Nhận ISP version từ U7, chuỗi "fw_ver-hw_ver"
      char ver_buf[20] = {0};
      memcpy(ver_buf, rx_frame.data,
             min((size_t)rx_frame.header.msk_regs.len, sizeof(ver_buf) - 1));
      char *dash = strchr(ver_buf, '-');
      if (dash) {
        *dash = '\0';
        strncpy(g_isp_fw_version, ver_buf, sizeof(g_isp_fw_version) - 1);
        strncpy(g_isp_hw_version, dash + 1, sizeof(g_isp_hw_version) - 1);
      }
      g_isp_version_received = true;
      dbg_main("\n[U6] === ISP VERSION RECEIVED === FW: %s, HW: %s\n",
               g_isp_fw_version, g_isp_hw_version);

      static bool bcast_twr_restore_done = false;
      if (!bcast_twr_restore_done) {
        bcast_twr_restore_done = true;
        g_bcast_twr_restore_at = millis() + 10000;
        g_bcast_twr_restore_pending = true;
        debug_beacon("[BCAST_TWR] ISP version received, schedule restore after 10 seconds");
      }
    }

    else if (rx_frame.header.msk_regs.add == HANDLE_LED_STATUS_RES.add) {
      Fag_mask_regs.FAG_HANDLE_LED_STATUS_RES = 0;
      memset((uint8_t *)&led_status_t, 0, sizeof(led_status_t));
      memcpy(&led_status_t, rx_frame.data, rx_frame.header.msk_regs.len);
      dbg_main("read led %X\n\r", rx_frame.header.msk_regs.add);
      dbg_main("read ledP %d\n\r", led_status_t.power);
      dbg_main("read ledI %d\n\r", led_status_t.internet);
      dbg_main("read ledDW %d\n\r", led_status_t.decawave);

    }

    else if (rx_frame.header.msk_regs.add == START_OTA.add) {
      write_ota_t start_ota;
      memset((uint8_t *)&start_ota, 0, sizeof(start_ota));
      memcpy(&start_ota, rx_frame.data, rx_frame.header.msk_regs.len);
      if (strcmp((char *)start_ota.data, "OK") == 0) {
        dbg_main("init ota ok");
        otaled_flag = ota_read_data;
      } else if (strcmp((char *)start_ota.data, "Update_comple") == 0) {
        dbg_main("Update_comple");
        g_u7_ota_status = "SUCCESS";
        report_UpdateOTA(true); // Thành công
      } else if (strcmp((char *)start_ota.data, "Update_fail") == 0) {
        dbg_main("Update_fail");
        g_u7_ota_status = "FAILED";
        report_UpdateOTA(false); // Thất bại (lỗi khi nạp)
      } else if (strcmp((char *)start_ota.data, "MD5_fail") == 0) {
        dbg_main("MD5_fail");
        g_u7_ota_status = "FAILED";
        report_UpdateOTA(false); // Thất bại sai checksum
      } else {
        dbg_main("init ota fail");
        Update_info.DW_update = false;
        otaled_flag = ota_null;
        report_UpdateOTA(false);
      }
    }

    else if (rx_frame.header.msk_regs.add == FRAGMENT_STATUS_RES.add) {
      fragment_status_t status_frag;
      memcpy(&status_frag, rx_frame.data, sizeof(fragment_status_t));
      dbg_main("Fragment status received: uint32 low = %08X",
               (uint32_t)status_frag.status);

      // Cập nhật biến global cho Web polling endpoint
      extern String g_beacon_ack_status;
      extern uint64_t g_beacon_ack_bits;
      g_beacon_ack_bits = status_frag.status;

      if (status_frag.status != 0) {
        sendmqtt_fragment_status(status_frag.status);

        // Kiểm tra bit 63 (Finish Flag) để dừng polling
        bool is_done = (status_frag.status & (1ULL << 63)) != 0;
        if (is_done) {
          g_beacon_ack_status = "DONE";
        } else {
          g_beacon_ack_status = "ACKED";
        }

        if (is_done && Fag_mask_regs.FAG_BEACON_RES == 1) {
          Fag_mask_regs.FAG_BEACON_RES =
              0; // Đã hoàn thành xong Task, dừng query
          dbg_main("Fragmentation Task FINISHED!");
        }
      }
    } else if (rx_frame.header.msk_regs.add == REQUEST_DATA_RES.add) {
      request_data_t req_data;
      memcpy(&req_data, rx_frame.data, sizeof(request_data_t));
      dbg_main("Received Request Data from U7, sending to Server (CMD 97)");
      sendmqtt_request_data(&req_data);

      // Luu vao bien global cho Web polling
      extern volatile bool g_request_data_ready;
      extern request_data_t g_last_request_data;
      memcpy(&g_last_request_data, &req_data, sizeof(request_data_t));
      g_request_data_ready = true;
    }
  }
}

extern void Task_Uart_rx(void);
extern volatile bool start_isp3080_ota;

uint32_t calc_file_crc32(File &f) {
  uint32_t crc = 0xFFFFFFFF;
  f.seek(0);
  uint8_t buf[256];
  while (f.available()) {
    int len = f.read(buf, sizeof(buf));
    for (int i = 0; i < len; i++) {
      crc ^= buf[i];
      for (int b = 0; b < 8; b++) {
        if (crc & 1)
          crc = (crc >> 1) ^ 0xEDB88320;
        else
          crc >>= 1;
      }
    }
  }
  f.seek(0);
  return ~crc;
}

void report_ISP3080_OTA_Status(uint8_t status, uint8_t percent = 0) {
  DynamicJsonDocument doc(512);
  doc["SerialID"] = Config_Device.Device.SerialID;
  doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
  doc["CMD"] = 96; // CMD OTA ISP3080
  doc["FileName"] = "isp3080.bin";

  if (status == 0) {
    doc["Reply"] = "Update_fail";
  } else if (status == 1) {
    doc["Reply"] = "Update_complete";
  } else if (status == 2) {
    doc["Reply"] = "Update_progress";
    doc["Progress"] = percent;
  }

  String _output;
  serializeJson(doc, _output);
  Mqtt_Handle.send_data(TopicDevice.c_str(), _output.c_str());
}

void Task_ISP3080_OTA_Loop() {
  if (!start_isp3080_ota)
    return;

  // Mở file firmware từ SPIFFS
  File f = SPIFFS.open("/isp3080.bin", FILE_READ);
  if (!f) {
    debug_OTA("[U6_OTA] Loi mo file /isp3080.bin");
    start_isp3080_ota = false;
    return;
  }

  uint32_t fw_size = f.size();
  uint32_t crc32_val = calc_file_crc32(f);

  long start_ota_time = millis();

  // Dọn sạch UART RX
  while (Serial1.available())
    Serial1.read();

  debug_OTA("[U6_OTA] Bat dau gui ENTER (Binary) fw_size: %lu\n", fw_size);

  g_ota_ack_enter_received = false;

  com_frame_t frame_enter;
  memset(&frame_enter, 0, sizeof(frame_enter));
  frame_enter.header.type = 0x04;           // write_ram
  frame_enter.header.msk_regs.add = 0x10A0; // ENTER
  frame_enter.header.msk_regs.len = sizeof(isp_ota_enter_t);

  isp_ota_enter_t enter_payload;
  enter_payload.fw_size = fw_size;
  enter_payload.crc32_val = crc32_val;
  memcpy(frame_enter.data, &enter_payload, sizeof(enter_payload));
  frame_enter.header.check_crc =
      SPI_master.calcCRC(frame_enter.data, frame_enter.header.msk_regs.len);
  Serial1.write((uint8_t *)&frame_enter,
                sizeof(frame_enter.header) + frame_enter.header.msk_regs.len);

  uint32_t t_start = millis();
  while (millis() - t_start < 50000) {
    Task_Uart_rx();    // Nhận ACK từ UART
    SPI_master.loop(); // Giữ các luồng khác không chết
    if (g_ota_ack_enter_received)
      break;
    delay(1);
  }

  if (!g_ota_ack_enter_received || !g_ota_ack_enter_status) {
    debug_OTA("[U6_OTA] ENTER Failed hoac Timeout. Huy OTA.");
    f.close();
    report_ISP3080_OTA_Status(0); // Báo lỗi lên MQTT
    start_isp3080_ota = false;
    return;
  }

  debug_OTA("[U6_OTA] Nhan ACK: ENTER_OK");

  uint32_t offset = 0;
  uint8_t buf[240];
  while (f.available()) {
    int len = f.read(buf, 240);
    if (len > 0) {
      g_ota_ack_data_received = false;

      com_frame_t frame_data;
      memset(&frame_data, 0, sizeof(frame_data));
      frame_data.header.type = 0x04;           // write_ram
      frame_data.header.msk_regs.add = 0x10A1; // DATA
      frame_data.header.msk_regs.len =
          sizeof(isp_ota_data_t); // Luôn gửi đủ struct hoặc chỉ phần có data

      isp_ota_data_t data_payload;
      memset(&data_payload, 0, sizeof(data_payload));
      data_payload.offset = offset;
      data_payload.len = len;
      memcpy(data_payload.data, buf, len);

      memcpy(frame_data.data, &data_payload, sizeof(data_payload));
      frame_data.header.check_crc =
          SPI_master.calcCRC(frame_data.data, frame_data.header.msk_regs.len);
      Serial1.write((uint8_t *)&frame_data,
                    sizeof(frame_data.header) + frame_data.header.msk_regs.len);

      uint32_t t_data = millis();
      while (millis() - t_data < 5000) {
        Task_Uart_rx();    // Đọc ACK
        SPI_master.loop(); // Giữ kết nối SPI
        if (g_ota_ack_data_received)
          break;
        delay(1);
      }

      if (!g_ota_ack_data_received || !g_ota_ack_data_status) {
        debug_OTA("%s", ("[U6_OTA] DATA Failed hoac Timeout tai offset " +
                       String(offset)).c_str());
        f.close();
        report_ISP3080_OTA_Status(0); // Báo lỗi

        // Gui ABORT
        com_frame_t frame_abort;
        memset(&frame_abort, 0, sizeof(frame_abort));
        frame_abort.header.type = 0x04;
        frame_abort.header.msk_regs.add = 0x10A3; // ABORT
        frame_abort.header.msk_regs.len = 0;
        frame_abort.header.check_crc = SPI_master.calcCRC(frame_abort.data, 0);
        Serial1.write((uint8_t *)&frame_abort, sizeof(frame_abort.header));

        start_isp3080_ota = false;
        return;
      }
      offset += len;
      if (offset % 4800 == 0) {
        uint8_t percent = (offset * 100) / fw_size;
        // Serial.printf("[U6_OTA] Tien do: %lu / %lu bytes (%d%%)\n", offset,
        //               fw_size, percent);
        report_ISP3080_OTA_Status(2, percent);
      }
    }
  }

  debug_OTA("[U6_OTA] Gui END chunk");
  g_ota_ack_end_received = false;

  com_frame_t frame_end;
  memset(&frame_end, 0, sizeof(frame_end));
  frame_end.header.type = 0x04;
  frame_end.header.msk_regs.add = 0x10A2; // END
  frame_end.header.msk_regs.len = 0;
  frame_end.header.check_crc = SPI_master.calcCRC(frame_end.data, 0);
  Serial1.write((uint8_t *)&frame_end, sizeof(frame_end.header));

  uint32_t t_end = millis();
  while (millis() - t_end < 30000) {
    Task_Uart_rx();
    SPI_master.loop();
    if (g_ota_ack_end_received)
      break;
    delay(1);
  }

  f.close();
  if (g_ota_ack_end_received && g_ota_ack_end_status) {
    debug_OTA(
        "[U6_OTA] HOAN TAT OTA ISP3080 THANH CONG! Thoi gian: %lu ms\n",
        millis() - start_ota_time);
    report_ISP3080_OTA_Status(1); // Báo hoàn tất MQTT trước khi reset U6
    g_isp_version_received = false; // Bắt buộc poll lại version từ U7

    // Chờ MQTT có thời gian publish gói Update_complete ra broker rồi reset U6.
    uint32_t mqtt_flush_start = millis();
    while (millis() - mqtt_flush_start < 2000) {
      Mqtt_Handle.mqtt_loop();
      delay(10);
    }
    debug_OTA("[U6_OTA] Reset U6 sau khi ISP3080 OTA complete.");
    ESP.restart();
  } else {
    debug_OTA("[U6_OTA] Xong file nhung khong nhan duoc ACK:END tu U7.");
    report_ISP3080_OTA_Status(0); // Báo lỗi
  }

  // Dọn dẹp file OTA của ISP3080 để giải phóng dung lượng SPIFFS cho U6
  SPIFFS.remove("/isp3080.bin");
  debug_OTA("[U6_OTA] Da xoa file /isp3080.bin khoi SPIFFS.");

  start_isp3080_ota = false;
}

void Task_Uart_rx(void) {
  if (Serial1.available()) {
    delay(10);
    size_t len = Serial1.available();
    // Giới hạn đọc tối đa 512 bytes, tránh tràn stack (VLA cũ có thể lên tới
    // 2048)
    static uint8_t buff[512];
    if (len > sizeof(buff))
      len = sizeof(buff);
    Serial1.readBytes(buff, len);
    dbg_main("do dai uart %d", len);
    // printf_hex("uart data", buff, len);
    Taks_Uart_rx(buff, len);
    // Serial.printf("\r\nFEED\r\n");
    extern volatile bool start_isp3080_ota;
    if (!start_isp3080_ota) {
      read_button_state();
    }
    // loop_button_control();
    u7_TimeCheck.ToEUpdate(30000);
  }
}

// void Task_debug_rx(void)
// {
//   if (Serial.available())
//   {
//     String data = Serial.readString();
//     // Serial.print(data);
//     if (strcmp(data.c_str(), "DEBUG=1") == 0)
//     {
//       dbg_main("active debug TDoA");
//       EEPROM.write(EEPROM_ADDR_DEBUG, 1);
//       EEPROM.commit();
//       debug_TDOA = 1;
//     }
//     else if (strcmp(data.c_str(), "DEBUG=0") == 0)
//     {
//       dbg_main("deactive debug TDoA");
//       EEPROM.write(EEPROM_ADDR_DEBUG, 0);
//       EEPROM.commit();
//       debug_TDOA = 0;
//     }
//   }
// }
void Task_debug_rx(void) {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim(); // loại bỏ \r \n và khoảng trắng

    // --- 1) Lệnh bật/tắt debug ---
    if (data.equalsIgnoreCase("DEBUG=1")) {
      dbg_main("active debug TDoA");
      EEPROM.write(EEPROM_ADDR_DEBUG, 1);
      EEPROM.commit();
      debug_TDOA = 1;
      return;
    } else if (data.equalsIgnoreCase("DEBUG=0")) {
      dbg_main("deactive debug TDoA");
      EEPROM.write(EEPROM_ADDR_DEBUG, 0);
      EEPROM.commit();
      debug_TDOA = 0;
      return;
    }

    // --- 2) Lệnh cấu hình CFG MAC/HOST/DEVID ---
    if (data.startsWith("CFG ")) {
      UartConfig::handleLine(data);
      return;
    }

    // --- 3) Lệnh hiển thị config hiện tại ---
    if (data.equalsIgnoreCase("SHOW")) {
      UartConfig::showConfig(Serial);
      return;
    }

    // --- 4) Không khớp cú pháp ---
    dbg_main("[UART] Unknown command: %s\n", data.c_str());
  }
}

// Đọc 1 dòng từ Serial1, cắt \r nếu có, timeout ms
static bool readLineSerial1(String &out, uint32_t timeout_ms = 500) {
  Serial1.setTimeout(timeout_ms);
  out = Serial1.readStringUntil('\n'); // không chứa '\n'
  // cắt \r ở cuối cho khớp handleLine() bên B
  while (out.endsWith("\r"))
    out.remove(out.length() - 1);
  return out.length() > 0;
}

// Gửi DEVID=<id> và chờ "OK DEVID=..."
static bool setPeerDeviceId(uint32_t id, uint8_t maxRetries = 3,
                            uint32_t respTimeoutMs = 800) {
  // Xoá rác còn tồn trong RX trước khi gửi
  while (Serial1.available())
    Serial1.read();

  for (uint8_t attempt = 1; attempt <= maxRetries; ++attempt) {
    // Serial.printf("[A] Push DEVID attempt %u...\n", attempt);
    Serial1.printf("DEVID=%lu\r\n", (unsigned long)id);

    uint32_t t0 = millis();
    while (millis() - t0 < respTimeoutMs) {
      if (Serial1.available()) {
        String line;
        if (!readLineSerial1(line, respTimeoutMs))
          continue;
        // mong đợi "OK DEVID=xxxx"
        if (line.startsWith("OK ") && line.indexOf("DEVID=") >= 0) {
          // Serial.printf("[A] Peer replied: %s\n", line.c_str());
          return true;
        }
        if (line.startsWith("ERR")) {
          // Serial.printf("[A] Peer ERR: %s\n", line.c_str());
          break; // retry lần sau
        }
      }
      delay(1);
    }
  }
  return false;
}

// Hỏi lại để xác nhận
// static bool queryPeerDeviceId(uint32_t &outId,
//                               uint8_t maxLines = 3,
//                               uint32_t respTimeoutMs = 500)
// {
//   while (Serial1.available())
//     Serial1.read();
//   Serial1.print("DEVID?\r\n");

//   for (uint8_t i = 0; i < maxLines; ++i)
//   {
//     String line;
//     if (!readLineSerial1(line, respTimeoutMs))
//       continue;
//     // B trả "OK DEVID=<value>"
//     int p = line.indexOf("DEVID=");
//     if (line.startsWith("OK ") && p >= 0)
//     {
//       const char *s = line.c_str() + p + 6;
//       // chấp nhận dec hoặc 0xHEX
//       char *endp = nullptr;
//       uint32_t val = (strncmp(s, "0x", 2) == 0 || strncmp(s, "0X", 2) == 0)
//                          ? strtoul(s, &endp, 16)
//                          : strtoul(s, &endp, 10);
//       if (endp && *endp == '\0')
//       {
//         outId = val;
//         return true;
//       }
//     }
//   }
//   return false;
// }

// SPIClass H_SPI(HSPI);
void Init_Setup(void) {
  // Device_check_reset_reason();
  Serial.begin(115200);
  dbg_main("U6 Version 14/4/2026");

  Serial.setTimeout(10);

  Serial1.setRxBufferSize(2048);
  Serial1.setTimeout(1000);
  Serial1.flush();
  Serial1.begin(921600, SERIAL_8N1, 34, 32);
  setup_button_control(); // button control U7 led

  // dbg_main("Hardware Ver: %s", HARDWARE_VERSION);
  // dbg_main("Firmware Ver: %s", FIRMWARE_VERSION);

  // EEPROM.begin(10);
  // debug_TDOA = EEPROM.read(0);
  EEPROM.begin(128); // mở rộng EEPROM
  debug_TDOA = EEPROM.read(
      EEPROM_ADDR_DEBUG); // dời cờ debug sang vùng cuối, tránh 0..41
  dbg_main("active: %d ", debug_TDOA);

  // uint32_t devId = 0;
  // if (eepromReadDeviceID(devId))
  // {
  //   Serial.printf("\n [CFG] DeviceID: 0x%08lX (%lu)\n", devId, devId);
  // }
  // else
  // {
  //   Serial.println("[CFG] DeviceID invalid (0 or 0xFFFFFFFF)");
  // }
  uint32_t devId = 0;
  if (eepromReadDeviceID(devId)) {
    // Serial.printf("\n [CFG] DeviceID: 0x%08lX (%lu)\n", devId, devId);

    // === Gửi set DeviceID sang ESP32-B ===
    if (!setPeerDeviceId(devId)) {
      // Serial.println("[A] Set DeviceID to B: FAILED (no OK within
      // timeout).");
    } else {
      // Xác nhận lại
      uint32_t confirm = 0;
      if (queryPeerDeviceId(confirm) && confirm == devId) {
        // Serial.printf("[A] OK, B confirmed DEVID=%lu\n", (unsigned
        // long)confirm);
      } else {
        // Serial.println("[A] WARN: cannot confirm DEVID? from B.");
      }
    }
  } else {
    // Serial.println("[CFG] DeviceID invalid (0 or 0xFFFFFFFF)");
  }

  SPI_master.begin();
  sd_handle_config();
  Config_Device.checkBaseConfig();
  Config_Device.checkDHCPconfig();
  //  my_ibeacon.checkBeaconConfig();
  // SD_Handle.begin();
  g_bcast_twr_net_queue = xQueueCreate(16, sizeof(bcast_twr_net_frame_t));
  if (g_bcast_twr_net_queue == NULL) {
    debug_beacon("[BCAST_TWR] ERROR: cannot create network queue");
  }
  SPI_master.reciver_callback(Taks_Spi_rx);

  // setCustomMACEarly(); // mở khi cần dùng MAC tùy chỉnh
  //  ===== (1) ĐẶT MAC RẤT SỚM từ EEPROM (hoặc dùng default nếu trống) =====
  uint8_t savedMac[6];
  if (eepromReadMac(savedMac)) {
    // đảm bảo gọi TRƯỚC ETH.begin()
    if (!setCustomMACEarly(savedMac)) {
      dbg_enthernet(
          "WARN: setCustomMACEarly(EEPROM) failed, dùng eFuse/base mặc định.");
    }
  } else {
    // fallback MAC mặc định nếu muốn
    // static const uint8_t U6_DEFAULT_MAC[6] = {0x8C, 0x1F, 0x64, 0xCE, 0xD0,
    // 0x05};
    static const uint8_t U6_DEFAULT_MAC[6] = {0x00, 0x00, 0x00,
                                              0x00, 0x00, 0x00};
    setCustomMACEarly(U6_DEFAULT_MAC);
  }

  handle_ethernet.eth_setup();
  //  setEthHostname(U6_HOSTNAME);
  //   ===== (3) ĐẶT HOSTNAME sau khi ETH.begin() =====
  String savedHost;
  if (eepromReadHostname(savedHost)) {
    setEthHostname(savedHost.c_str()); // dùng hàm trong mac_hostname_u6.h
    dbg_main("[CFG] Loaded Hostname (EEPROM): %s\n", savedHost.c_str());
  } else {
    setEthHostname(U6_HOSTNAME); // fallback mặc định
    // Serial.printf("[CFG] Using default Hostname: %s\n", U6_HOSTNAME);
    dbg_main("[CFG] Loaded Hostname default: %s\n", U6_HOSTNAME);
  }

  Mqtt_Handle.mqtt_setup();
  FTP_Handle.begin(Config_Internet.FTP.Server, Config_Internet.FTP.User,
                   Config_Internet.FTP.Pass);
  WebServerInit();
  // Khởi tạo nút trên GPIO2, debounce 30ms, ngưỡng giữ 10s
  factoryButtonBegin(2, 30, 10000);
}

static void poll_read_led() {
  static uint32_t last_ms = 0;
  uint32_t now = millis();
  if (now - last_ms >= 10000) { // 10 giây
    last_ms = now;
    // Chỉ yêu cầu task xử lý UART cập nhật; không truy cập tx_frame tại loopTask.
    FlagcheckLed = true;
  }
}

// char databuffer[100];
void codeTaskInternetCore0(void *parameter) // Task kết nối Internet
{
  //	xTaskCreate(&readspi_task, "readSPI", 4096, NULL, 2, NULL);

  while (1) {
    handle_ethernet.eth_loop();
    // Luôn ưu tiên MQTT keepalive/callback trước lưu lượng TCP BSS-TWR.
    Mqtt_Handle.mqtt_loop();
    mqtt_send_isp_version(); // Gửi ISP version CMD 24 mỗi 20s

    // Chỉ gửi tối đa 1 frame mỗi vòng để TWR không chiếm độc quyền network task.
    bcast_twr_net_frame_t twr_frame;
    if (g_bcast_twr_net_queue != NULL &&
        xQueueReceive(g_bcast_twr_net_queue, &twr_frame, 0) == pdPASS) {
      _handle_tcp.sendTWR(twr_frame.data, twr_frame.len);
    }

    _handle_tcp
        .Twr_Client_Socket_Stream_2011_Handle(); // TCP port 2011 cho BSS-TWR
    _handle_tcp.Tdoa_Client_Socket_Stream1_Handle();
    _handle_tcp.Tdoa_Client_Socket_Stream2_Handle();
    _handle_tcp.Tdoa_Client_Socket_Stream_1120_Handle();
    // loop_button_control(); // button control U7 led
    // Serial.printf("loop internet\r\n");

    if (ETH.linkUp() && ETH.localIP() != INADDR_NONE) {
      static bool name_ok = false;
      if (!name_ok) {
        U6NameSvc_startOnce(); // mDNS + NBNS + LLMNR
        name_ok = true;
        // printEthInfo();
      }
    }

    if (!Update_info.DW_update) {
      // Webserverloop();
      uint32_t num = SPI_master.Buff_is_available();
      if (num) {
        uint8_t databuff[num];
        memset(databuff, 0, num);
        SPI_master.readBufferDMA(databuff, num);
        debug_SPI("last byte: %c", (char)databuff[num - 1]);
        debug_SPI("len:%d", num);

        if ((uint8_t)databuff[num - 1] != 13) {
          uint16_t i = 0;
          for (i; i <= num; i++) {
            // Serial.printf("%c", (char *)databuff[i]);
            if ((uint8_t)databuff[i] == 13) {
              // dbg_main("rac %c, %d/%d", (char *)databuff[i], i, num);
              num = i;
              break;
            }
          }
        }

        _handle_tcp.sendTCP((uint8_t *)databuff, num);

        debug_TCP("send tcp: %d\n", num);
        debug_TCP("send tcp: %s\n", (char *)databuff);
      }
    }

    delay(1);
  }
}

void printf_hex(char *header, uint8_t *data, unsigned int len) {
  debug_SPI("%s: ", header);
  for (unsigned int i = 0; i < len; i++)
    debug_SPI("%d ", data[i]);
  debug_SPI("");
}

void codeTaskProcessCore1(void *parameter) {
  while (1) {
    // loop_button_control(); // button control U7 led
    if (ota_TimeCheck.ToEExpired()) {
      check_OTA_DW();

      FlagcheckLed = true; // Refresh LED state to U7 every 3s
      Fag_mask_regs.FAG_DW_ANT_DELAY_RES = 1;
      Fag_mask_regs.FAG_DW_CONFIG_RES = 1;
      Fag_mask_regs.FAG_DW_CONFIG_TX_RES = 1;
      Fag_mask_regs.FAG_MASTER_RES = 1;
      Fag_mask_regs.FAG_SerialID_RES = 1;
      Fag_mask_regs.FAG_MASTER_ACCESS_RES1 = 1;
      Fag_mask_regs.FAG_MASTER_ACCESS_RES2 = 1;
      Fag_mask_regs.FAG_MASTER_ACCESS_RES3 = 1;
      Fag_mask_regs.FAG_MASTER_ACCESS_RES4 = 1;
      Fag_mask_regs.FAG_VERSION_RES = 1;
      // Không ghi đè FAG_BEACON_RES khi đang chờ gửi (=2), tránh mất config CMD 20
      if (Fag_mask_regs.FAG_BEACON_RES != 2) {
        Fag_mask_regs.FAG_BEACON_RES = 1;
      }
    }

    // Poll ISP version từ U7 (Mỗi 5s nếu chưa nhận được)
    static unsigned long last_isp_poll = 0;
    if (!g_isp_version_received && millis() - last_isp_poll > 5000) {
      last_isp_poll = millis();
      // Serial.println("[U6] Requesting ISP version from U7...");
      SPI_master.readISPVersion();
    }

    if (!FlagOTA) {
      if (start_isp3080_ota) {
        Task_ISP3080_OTA_Loop(); // Độc chiếm luồng cho OTA
      } else {
        Task_Uart_rx(); // Nhận binary bình thường
      }
    } else {
      u7_TimeCheck.ToEUpdate(180000);
    }

    // Thực thi restore ngoài callback nhận UART và sau khi U7/ISP đã ổn định.
    if (g_bcast_twr_restore_pending &&
        (long)(millis() - g_bcast_twr_restore_at) >= 0) {
      g_bcast_twr_restore_pending = false;
      restore_bcast_twr_after_boot();
    }

    SPI_master.loop();

    // Task_debug_rx();
    if (!Update_info.DW_update) {

      SPI_master.setConfigDW();
      SPI_master.readConfigDW();
      SPI_master.setConfigBeacon();
    } else {
      u7_TimeCheck.ToEUpdate(180000);
    }

    OTA_Main_loop();
    OTA_DW_loop();

    if (u7_TimeCheck.ToEExpired()) {
      reset_dw_chip();
      u7_TimeCheck.ToEUpdate(30000);
    }

    if (ESPRebootTo.ToEExpired()) {
      dbg_main("Esp Rebooting...");
      ESP.restart();
    }

    delay(1);
  }
}

TaskHandle_t InternetCore0;
TaskHandle_t ProcessCore1;

void reset_dw_chip() { // Mở lại để reset lại U7 khi bị treo
  pinMode(SD_CARD_POWER, OUTPUT);
  digitalWrite(SD_CARD_POWER, LOW);
  digitalWrite(SD_CARD_POWER, LOW);
  delay(1500);
  // digitalWrite(SD_CARD_POWER, HIGH);
  // digitalWrite(SD_CARD_POWER, HIGH);
  // delay(1000);
  pinMode(SD_CARD_POWER, INPUT);
  dbg_main("\r\nRESET u7\r\n");
}

void setup() {
  // 20240623 - reset chip U7
  delay(5000);
  // reset_dw_chip();
  Init_Setup();

  // Task xử lý phần giao tiếp internet
  xTaskCreatePinnedToCore(codeTaskInternetCore0, "TaskInternetCore0", 15 * 1024,
                          NULL, 2, &InternetCore0, 0);
  // Task sử lý chung
  xTaskCreatePinnedToCore(codeTaskProcessCore1, "TaskProcessCore1", 15 * 1024,
                          NULL, 1, &ProcessCore1, 1);
}

void loop() {
  loop_button_control(); // button control U7 led
  Task_debug_rx();
  poll_read_led();
  factoryButtonTask();
  // Serial.printf("loop internet\r\n");
}
