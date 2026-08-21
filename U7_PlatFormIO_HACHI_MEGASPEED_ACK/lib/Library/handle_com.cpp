#include "handle_com.h"

// Biến lưu ISP3080 version (được đọc 1 lần lúc boot)
char g_isp_fw_ver[10] = "0.00";
char g_isp_hw_ver[10] = "0.00";
#include "handle_ISP3080.h"
#include "handle_com_regs.h"
#include "handle_decawave.h"
#include "handle_spifs.h"
#include "handle_spis.h"
#include "handle_status.h"

// #include "freertos/semphr.h"
#include "main.h"
#include <TimeOutEvent.h>

SemaphoreHandle_t xMutex = NULL;
dw_two_way_t two_way;
volatile bool Flagbuffer;
TimeOutEvent ESPRebootTo(0);
beacon_cfg_t g_beacon_cfg;

TimeOutEvent fragPollTimer(0);
TimeOutEvent fragTimeoutTimer(0); // Timeout 1 phút
bool is_polling_fragment = false;
uint64_t last_fragment_status = 0;

ComHandle::ComHandle(/* args */) {
  // xMutex = xSemaphoreCreateMutex();
}
ComHandle::~ComHandle() {}

void distant(double Distance) {
  extern bool g_bcast_twr_active;
  extern uint8_t temp_id_mode;
  // ==== GUARD: Không gửi DS-TWR result khi BSS-TWR đang bật ====
  if (g_bcast_twr_active) {
    dbg_com("distant() BLOCKED: BSS-TWR is active!");
    return;
  }

  if (Distance < 0) {
    dbg_com("distant: TAG NOT FOUND -> 0xFFFFFFFF");
    two_way.distance = 0xFFFFFFFF;
  } else {
    dbg_com("distant : %lu", (uint32_t)(Distance * 100));
    two_way.distance = (uint32_t)(Distance * 100);
  }

  if (temp_id_mode == 5) {
    // Mode 5 (SS-TWR Base-Tag): Gửi qua SPI CSV → TCP port 2011
    char spi_buf[64];
    int spi_len = snprintf(spi_buf, sizeof(spi_buf), "D,%lu,%lu:\r\n",
                           (unsigned long)two_way.deviceID,
                           (unsigned long)two_way.distance);
    Handle_Com.GiveBuff((uint8_t *)spi_buf, (uint8_t)spi_len);
  } else {
    // CMD 8 (DS-TWR Base-Base): Gửi UART frame về U6 để gọi sendmqtt_distance()
    frame_work_t tx_frame;
    memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
    tx_frame.header.type = read_distant;
    tx_frame.header.msk_regs = SET_TWO_WAY;
    memcpy((uint8_t *)tx_frame.data, &two_way, sizeof(two_way));
    tx_frame.header.check_crc =
        Handle_SPIs.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
    Serial1.write((uint8_t *)&tx_frame,
                  sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
    dbg_com("distant: Sent UART frame to U6 (deviceID=%lu, dist=%lu)",
            (unsigned long)two_way.deviceID,
            (unsigned long)two_way.distance);
  }
}



// send_twr_result() đã deprecated — TWR Result giờ gửi qua SPI→TCP port 2011


void ComHandle::GiveBuff(uint8_t *data, uint8_t length) {
  // xSemaphoreTake(xMutex, 1);
  // if(Flagbuffer)
  // 	return;
  // Flagbuffer = true;
  memcpy(this->com_buff.data[this->com_buff.id_wr].array, data, length);
  this->com_buff.data[this->com_buff.id_wr].length = length;
  if (++this->com_buff.id_wr >= COMMUNICATION_NUM_BUFFER) {
    this->com_buff.id_wr = 0;
  }
  // Flagbuffer = false;
  // xSemaphoreGive(xMutex);
}

bool ComHandle::CheckBuff(void) {
  if (this->com_buff.id_rd != this->com_buff.id_wr)
    return true;
  return false;
}

int ComHandle::TakeBuff(uint8_t *data) {
  // xSemaphoreTake(xMutex, 1);
  // if(Flagbuffer)
  // 	return -1;
  // Flagbuffer = true;
  if (this->CheckBuff() == false) {
    return -1;
  }

  uint8_t length = this->com_buff.data[this->com_buff.id_rd].length;
  memcpy(data, this->com_buff.data[this->com_buff.id_rd].array, length);
  /*xóa trống*/
  this->com_buff.data[this->com_buff.id_rd].length = 0;
  memset(this->com_buff.data[this->com_buff.id_rd].array, 0x00,
         COMMUNICATION_LENGHT_BUFFER);
  if (++this->com_buff.id_rd >= COMMUNICATION_NUM_BUFFER)
    this->com_buff.id_rd = 0;

  // Flagbuffer = false;
  // xSemaphoreGive(xMutex);
  return length;
}

void ComHandle::process_write_ram(mask_regs_t *regs, uint8_t *data) {
  switch (regs->add) {
  case 0x1000: // địa chỉ của trạng thái
  {
    handle_led_status_t status;
    memcpy((uint8_t *)&status, data, regs->len);
    dbg_com("led internet %d", status.internet);
    dbg_com("led power %d", status.power);
    // if (stat_btn)
    {
      led_status.internet((status_ledINTERNET)status.internet);
      led_status.power((status_ledPower)status.power);
    }
    break;
  }

  case 0x1001: // địa chỉ của ra lệnh xóa file config
  {
    dbg_com("len %d", regs->len);
    dbg_com("data 0x%02X", data[0]);

    handle_last_update_t del;
    memcpy((uint8_t *)&del, data, regs->len);
    dbg_com("data %d", del.flag_del);
    if (del.flag_del == true)
      parametter_dw.del(FILE_PARAMETTER_DW);
    break;
  }

  case 0x1004: // reset
  {
    reset_t Reset;
    memcpy((uint8_t *)&Reset, data, regs->len);
    if (Reset.reset == 2) {
      // dbg_com("reset dw %d", Reset.timeout);
      dbg_dw("___RESET DW____");
      digitalWrite(RESET_CLKDW, LOW);
      delay(100);
      digitalWrite(RESET_CLKDW, HIGH);
    }

    else if (Reset.reset == 3 || Reset.reset == 1) {
      dbg_dw("reset %d", Reset.timeout);
      // delay(Reset.timeout);
      delay(10);
      ESP.restart();

      // ESPRebootTo.ToEUpdate((uint32_t)Reset.timeout);
    }
    break;
  }
  default:
    break;
  }
}

int8_t ComHandle::process_raw_data(frame_work_t *data) {
  if (data == NULL)
    return -1;
  memcpy((uint8_t *)&this->com_frame, (uint8_t *)data,
         sizeof(this->com_frame.header) + data->header.msk_regs.len);

  // uint16_t check_crc =
  // Handle_SPIs.calcCRC(this->com_frame.data,this->com_frame.header.msk_regs.len);
  // // if(this->com_frame.header.type == Infor_OTA)
  // // {
  // 	dbg_com("type %d", this->com_frame.header.type);
  // 	dbg_com("check_crc %X", this->com_frame.header.check_crc);
  // 	dbg_com("msk regs add %X", this->com_frame.header.msk_regs.add);
  // 	dbg_com("msk regs len %d", this->com_frame.header.msk_regs.len);
  // 	dbg_com("msk regs data");
  // 	for(int i = 0 ; i < this->com_frame.header.msk_regs.len;i++){
  // 		dbg_com("%X ", this->com_frame.data[i]);
  // 	}

  // }

  // if(check_crc != this->com_frame.header.check_crc)
  // return -1;

  // xử lý data
  switch (this->com_frame.header.type) {

  case read_config: {
    memset(this->com_frame.data, 0, COMMUNICATION_LENGHT_BUFFER);
    // 20240622
    parametter_dw.read(FILE_PARAMETTER_DW, this->com_frame.header.msk_regs.add,
                       this->com_frame.data);
    dbg_com("read_config %s", (char *)this->com_frame.data);
    dbg_com("read data \n\r");
    // for (int i = 0; i < this->com_frame.header.msk_regs.len; i++)
    // 	Serial.printf("%X ", this->com_frame.data[i]);
    delay(1);
    return 1;
  }

    extern bool is_sending_config;

  case write_config: {
    // 1. Nếu là lệnh BEACON_CONFIG (0x300A)
    if (this->com_frame.header.msk_regs.add == BEACON_CFG_RES.add) {
      // 2. Tắt flag đồng bộ ISP3080 ngay lập tức để chặn các chu kỳ lặp nếu có
      is_sending_config = true;

      dbg_com("Recv BEACON_CFG (len=%d)", this->com_frame.header.msk_regs.len);
      memset(&g_beacon_cfg, 0, sizeof(beacon_cfg_t));
      memcpy(&g_beacon_cfg, this->com_frame.data,
             this->com_frame.header.msk_regs.len);

      // --- [IN LOG KIỂM TRA] ---
      dbg_com("--- [BEACON CONFIG FULL DUMP] ---");
      // 4. Timer
      dbg_com("[Timer] Motion:%u Stand:%u Sleep1:%u Sleep2:%u Sleep3:%u",
              g_beacon_cfg.val_motion, g_beacon_cfg.val_stand,
              g_beacon_cfg.val_sleep1, g_beacon_cfg.val_sleep2,
              g_beacon_cfg.val_sleep3);
      // 5. Mode
      dbg_com("[Mode] Mode1:%u Mode2:%u Mode3:%u", g_beacon_cfg.val_mode1,
              g_beacon_cfg.val_mode2, g_beacon_cfg.val_mode3);
      // 6. Battery
      dbg_com("[Batt] Def:%u High:%u Inc:%u Dec:%u",
              g_beacon_cfg.val_batt_default, g_beacon_cfg.val_batt_high,
              g_beacon_cfg.val_batt_inc, g_beacon_cfg.val_batt_dec);
      // 6. ID
      dbg_com("[ID] Last: %02X%02X%02X%02X%02X", g_beacon_cfg.val_id_last[0],
              g_beacon_cfg.val_id_last[1], g_beacon_cfg.val_id_last[2],
              g_beacon_cfg.val_id_last[3], g_beacon_cfg.val_id_last[4]);
      dbg_com("[ID] Serial: %02X%02X%02X%02X%02X", g_beacon_cfg.SerialID[0],
              g_beacon_cfg.SerialID[1], g_beacon_cfg.SerialID[2],
              g_beacon_cfg.SerialID[3], g_beacon_cfg.SerialID[4]);
      dbg_com("[ID] New: %02X%02X%02X%02X%02X", g_beacon_cfg.val_id_new[0],
              g_beacon_cfg.val_id_new[1], g_beacon_cfg.val_id_new[2],
              g_beacon_cfg.val_id_new[3], g_beacon_cfg.val_id_new[4]);
      dbg_com("[ID] Mode: %u", g_beacon_cfg.val_id_mode);
      dbg_com("[ID] Change Flag: %u", g_beacon_cfg.val_id_change);
      // 7. UWB
      dbg_com("[UWB] Chan:%u Plen:%u Pac:%u TxC:%u RxC:%u",
              g_beacon_cfg.uwb_chan, g_beacon_cfg.uwb_plen,
              g_beacon_cfg.uwb_pac, g_beacon_cfg.uwb_txcode,
              g_beacon_cfg.uwb_rxcode);
      dbg_com("[UWB] SFD:%u Rate:%u PhrM:%u PhrR:%u SFDTO:%u",
              g_beacon_cfg.uwb_sfdtype, g_beacon_cfg.uwb_datarate,
              g_beacon_cfg.uwb_phrmode, g_beacon_cfg.uwb_phrrate,
              g_beacon_cfg.uwb_sfdto);
      dbg_com("[UWB] STS:%u STSLen:%u PDOA:%u", g_beacon_cfg.uwb_stsmode,
              g_beacon_cfg.uwb_stslen, g_beacon_cfg.uwb_pdoa);
      dbg_com("-------------------------------------");

      if (set_beacon_config_to_nrf(&g_beacon_cfg)) {
        dbg_com("ISP3080 Sync: OK");
        is_polling_fragment = true; // Kích hoạt U7 tự động hỏi fragment status
        fragPollTimer.ToEUpdate(2000);     // 2s hỏi 1 lần
        fragTimeoutTimer.ToEUpdate(60000); // Hỏi tối đa trong 1 phút
        last_fragment_status = 0;
      } else {
        dbg_com("ISP3080 Sync: FAIL");
      }

      // 3. Lu\u00f4n x\u00f3a gi\u00e1 tr\u1ecb struct t\u1ea1m sau khi \u0111\u00e3 x\u1eed l\u00fd (G\u1eedi xong ho\u1eb7c L\u1ed7i)
      memset(&g_beacon_cfg, 0, sizeof(beacon_cfg_t));
      dbg_com("U7 Local Struct: CLEARED");

      memset(this->com_frame.data, 0, COMMUNICATION_LENGHT_BUFFER);
      return 0; // Đã xử lý xong
    }

    dbg_com("write_config %X", this->com_frame.header.msk_regs.add);
    parametter_dw.save(FILE_PARAMETTER_DW, this->com_frame.header.msk_regs.add,
                       this->com_frame.data);
    memset(this->com_frame.data, 0, COMMUNICATION_LENGHT_BUFFER);
    FlagReadconfig = true;
    return 0;
  }

  case read_ram: {
    dbg_com("read_ram");
    if (this->com_frame.header.msk_regs.add == 0x1005) {

      strcpy((char *)this->com_frame.data, FIRMWARE_VERSION);
      strcat((char *)this->com_frame.data, "-");
      strcat((char *)this->com_frame.data, HARDWARE_VERSION);
      dbg_com("version: %s", (char *)this->com_frame.data);
      // memccpy((uint8_t*)&version.HW_VR, HARDWARE_VERSION,
      // sizeof(version.HW_VR));
      return 1;
    }

    else if (this->com_frame.header.msk_regs.add == 0x1000) {
      handle_led_status_t status_led;
      status_led.internet = (uint8_t)internet_ready_f;
      status_led.power = (uint8_t)power_ready_f; // TODO led
      status_led.decawave = (uint8_t)decawave_ready_f;
      memset(this->com_frame.data, 0, COMMUNICATION_LENGHT_BUFFER);
      memcpy((uint8_t *)this->com_frame.data, (uint8_t *)&status_led,
             sizeof(handle_led_status_t));
      return 1;
    }

    else if (this->com_frame.header.msk_regs.add == 0x1009) {
      // U6 hỏi ISP version → U7 chủ động đọc lại từ ISP3080 để luôn có bản mới nhất
      extern char g_isp_fw_ver[10];
      extern char g_isp_hw_ver[10];
      read_isp_version(g_isp_fw_ver, sizeof(g_isp_fw_ver), g_isp_hw_ver, sizeof(g_isp_hw_ver));

      char ver_buf[20];
      snprintf(ver_buf, sizeof(ver_buf), "%s-%s", g_isp_fw_ver, g_isp_hw_ver);
      memset(this->com_frame.data, 0, COMMUNICATION_LENGHT_BUFFER);
      strcpy((char *)this->com_frame.data, ver_buf);
      dbg_com("ISP version: %s", ver_buf);
      return 1;
    }

    else if (this->TakeBuff(this->com_frame.data) > 0) {
      dbg_com("read_ram %s", (char *)this->com_frame.data);
      return 1;
    }

    return -1;
  }

  case write_ram: {
    // dbg_com("type %d", this->com_frame.header.type);
    // dbg_com("add %X", this->com_frame.header.msk_regs.add);
    // dbg_com("check_crc %X", this->com_frame.header.check_crc);
    // dbg_com("write_ram %s", (char *)this->com_frame.data);
    this->process_write_ram(&this->com_frame.header.msk_regs,
                            this->com_frame.data);
    break;
  }

  case infor_ota: {
    write_ota_t init_ota;
    memset(&init_ota, 0, sizeof(init_ota));
    memcpy(&init_ota, com_frame.data, this->com_frame.header.msk_regs.len);
    dbg_com("data:%s", init_ota.data);
    if (strcmp((char *)init_ota.data, "init_ota") == 0) {
      dbg_com("init_OTA");
      if (parametter_dw.createNewFileOTA(FWOTA)) {
        frame_work_t frame_work_ota;
        memset(&frame_work_ota, 0, sizeof(frame_work_ota));
        frame_work_ota.header.type = infor_ota;
        frame_work_ota.header.msk_regs.add = START_OTA.add;
        write_ota_t start_ota;
        memset((uint8_t *)&start_ota, 0, sizeof(start_ota));
        start_ota.size_data = strlen("OK");
        memcpy((uint8_t *)&start_ota.data, "OK", start_ota.size_data);
        frame_work_ota.header.msk_regs.len =
            start_ota.size_data + sizeof(start_ota.size_data);
        memcpy((uint8_t *)&frame_work_ota.data, &start_ota,
               frame_work_ota.header.msk_regs.len);
        frame_work_ota.header.check_crc = Handle_SPIs.calcCRC(
            frame_work_ota.data, frame_work_ota.header.msk_regs.len);
        Serial1.write((uint8_t *)&frame_work_ota,
                      sizeof(frame_work_ota.header) +
                          frame_work_ota.header.msk_regs.len);
        FlagOTA = true;
        dbg_com("create file OTA ok");
        checktimeOTA.ToEUpdate(10000);
      }
    } else
      dbg_com("init_OTA FAIL");
    return 0;
  }

  case read_distant: {
    memcpy((uint8_t *)&two_way, this->com_frame.data,
           this->com_frame.header.msk_regs.len);
    dbg_com("deviceID: %lu", two_way.deviceID);
    // GUARD: Không khởi động DS-TWR khi BSS-TWR đang bật
    if (g_beacon_cfg.enable_bcast_twr) {
      dbg_com("twr_start BLOCKED: BSS-TWR active!");
      return 0;
    }
    Handle_Dw.twr_start(two_way.deviceID, distant);
    return 0;
  }
  }
  return -1;
}

void ComHandle::loop_polling_fragment() {
  if (!is_polling_fragment)
    return;

  // Dừng quá trình hỏi khi hết thời gian Timeout (1 phút)
  if (fragTimeoutTimer.ToEExpired()) {
    is_polling_fragment = false;
    dbg_com("Polling STOPPED - Timeout 1 minute reached");
    dbg_com("%s", "\r\n[U7] Polling STOPPED - Timeout 1 minute reached");
  }
}

ComHandle Handle_Com;
