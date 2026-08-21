
#include "OTA.h"
#include "DataBase.h"
#include "FtpClientUpdate.h"
#include "Handle_FTP.h"
#include "handle_com_regs.h"
#include "handle_config.h"
#include "handle_mqtt.h"
#include "handle_spi_master.h"
#include "handle_wifi.h"

/* CRC16 dùng chung polynomial 0x8408 (giống calcCRC trong handle_spi_master) */
static uint16_t ota_calcCRC16(const uint8_t *data, size_t size) {
  uint16_t CrcPoly = 0x8408;
  uint16_t crc = 0;
  for (size_t j = 0; j < size; j++) {
    crc ^= data[j];
    for (int i = 0; i < 8; i++) {
      uint8_t carry = crc & 1;
      crc >>= 1;
      if (carry) crc ^= CrcPoly;
    }
  }
  return crc;
}

// #include <nRTLS_U6.ino>
extern void IRAM_ATTR isr();
Update_TypeDef Update_info;
String g_u7_ota_status = "IDLE";
otaled_flag_t otaled_flag = ota_null;
bool FlagOTA = false;
bool FlagDownloand = false;
#define size_block_pcs 1024
size_t size_block = size_block_pcs; // kích thước dữ liệu chuẩn bị gửi xuống
write_ota_t write_data_ota;

void check_OTA_DW() {
  // Không xóa file firmware cũ tại boot/trước OTA ISP.
  // File sẽ được dọn sau khi OTA kết thúc success/fail/timeout để tránh
  // SPIFFS.remove() block CPU đúng thời điểm bắt đầu OTA.
  Update_info.DW_update = false;
  otaled_flag = ota_null;
}

void OTA_Main_update(char *FName, char *Path, char *MD5) {
  strcpy(Update_info.Path, Path);
  strcpy(Update_info.Fname, FName);
  strcpy(Update_info.Md5, MD5);

  Update_info.Main_update = true;
  debug_OTA("Start update OTA for board main");
}

void OTA_DW_update(char *FName, char *Path, char *MD5) {
  strcpy(Update_info.Path, Path);
  strcpy(Update_info.Fname, FName);
  strcpy(Update_info.Md5, MD5);

  debug_OTA("Start download OTA for board DW");
  //    SPI_master.disable_interrup();
  if (FTP_Handle.download(Update_info.Path, Update_info.Fname,
                          Update_info.Md5)) {
    debug_OTA("FTP download OK");
    Update_info.DW_update = true;
    otaled_flag = ota_send_init;
  } else {
    debug_OTA("FTP download ERROR");
    Update_info.DW_update = false;
    report_UpdateOTA(false);
  }
  // SPI_master.enable_interrup();
}

void OTA_Main_loop(void) {
  if (Update_info.Main_update == false)
    return;
  Update_info.Main_update = false;

  String host = Config_Internet.FTP.Server;
  String user = Config_Internet.FTP.User;
  String pass = Config_Internet.FTP.Pass;
  // String path = String((char *)Update_info.Path);
  String path = String((char *)Update_info.Path) + "/" +
                String((char *)Update_info.Fname);
  String md5_file = String((char *)Update_info.Md5);

  debug_OTA("%s", host.c_str());
  debug_OTA("%s", user.c_str());
  debug_OTA("%s", pass.c_str());
  debug_OTA("%s", path.c_str());
  debug_OTA("%s", md5_file.c_str());
  debug_OTA("");

  if (FtpClientUpdate.update(host, user, pass, path, md5_file)) {
    debug_OTA("FTP Update Main OK");
    report_UpdateOTA(true);
    debug_OTA("---------");
    ESPRebootTo.ToEUpdate(5000);
    debug_OTA("---------");
  } else {
    uint8_t oat_err_code = FtpClientUpdate.getError();
    debug_OTA("FTP Update Main Fail %d", oat_err_code);
    report_UpdateOTA(false);
  }
}

void OTA_DW_loop() {
  while (Update_info.DW_update) {
    // debug_OTA("Update DW start");
    String md5_file = String((char *)Update_info.Md5);
    static uint8_t retry = 0;
    static TimeOutEvent TimeOut(0);
    static bool start = 0;       // cờ đọc file firmware
    static size_t file_size = 0; // kích thước của file

    uint8_t buff_data[size_block + 1]; // buff tạm để chứa dữ liệu
    com_frame_t frameota;

    // static write_ota_t write_ota;

    switch (otaled_flag) {
    case ota_send_init: {
      retry = 0; // Reset retry cho lần OTA mới
      debug_OTA("ota_send_init");
      file_size = sd_size_file(OTSD_Firmware_LED);
      debug_OTA("size file %lu", file_size);
      if (!file_size) {
        Update_info.DW_update = false;
        otaled_flag = ota_null;
        return;
      }

      // start_ota_t ota_infor;
      write_ota_t write_init;
      memset(&write_init, 0, sizeof(write_init));
      write_init.size_data = strlen("init_ota");
      memcpy(write_init.data, "init_ota", write_init.size_data);
      memset(&frameota, 0, sizeof(frameota));
      frameota.header.type = infor_ota;
      frameota.header.msk_regs.add = START_OTA.add;
      frameota.header.msk_regs.len =
          write_init.size_data + sizeof(write_init.size_data);
      memcpy(frameota.data, &write_init, frameota.header.msk_regs.len);
      frameota.header.check_crc =
          SPI_master.calcCRC(frameota.data, frameota.header.msk_regs.len);
      debug_OTA("crc:%d  ", frameota.header.check_crc);
      Serial1.write((uint8_t *)&frameota,
                    sizeof(frameota.header) + frameota.header.msk_regs.len);
      TimeOut.ToEUpdate(2000);
      otaled_flag = ota_null;
      return;
    }

    case ota_read_data:
      if ((start = sd_read_block_file(start, OTSD_Firmware_LED, buff_data,
                                      size_block)) == true) {
        memset(&write_data_ota, 0, sizeof(write_data_ota));

        if (file_size >= size_block)
          write_data_ota.size_data = size_block;
        else
          write_data_ota.size_data = file_size;

        memcpy((uint8_t *)write_data_ota.data, buff_data,
               write_data_ota.size_data);

        file_size -= size_block;
        debug_OTA("file size con lai:%d", file_size);
        // for (int i = 0; i < write_ota.size_data; i++)
        //     Serial.printf("%c", (char *)write_ota.data[i]);
        otaled_flag = ota_send_size;
        FlagOTA = true;
      } else {
        /* xoá cờ đọc file firmware*/
        debug_OTA("da read fw xong");
        start = 0;
        memset(&write_data_ota, 0, sizeof(write_data_ota));
        write_data_ota.size_data = 0xFAFA;

        // Tính MD5 TRƯỚC KHI BÁO U7 (vì tính MD5 quét SD Card mất ~900ms)
        FTP_Handle.calculator_md5(OTSD_Firmware_LED, Update_info.Md5);
        debug_OTA("MD5:%s", Update_info.Md5);
        strcpy((char *)&write_data_ota.data, Update_info.Md5);

        // Báo qua U7 chuẩn bị nhận MD5 (qua SIZE_OTA) - CÓ RETRY, KHÔNG DÙNG
        // String
        Serial1.setTimeout(2000);
        bool md5_sent = false;
        char ack_buf[32];
        for (int md5_retry = 0; md5_retry < 10; md5_retry++) {
          debug_OTA("send SIZE_OTA:%d (retry %d)", write_data_ota.size_data,
                    md5_retry);
          // Xả sạch buffer RX trước khi gửi (tránh đọc nhầm data cũ)
          while (Serial1.available())
            Serial1.read();

          Serial1.printf("SIZE_OTA:%d\n", write_data_ota.size_data);

          // Đọc phản hồi bằng char buffer (KHÔNG dùng String tránh heap crash)
          memset(ack_buf, 0, sizeof(ack_buf));
          size_t ack_len =
              Serial1.readBytesUntil('\n', ack_buf, sizeof(ack_buf) - 1);
          // Trim \r nếu có
          while (ack_len > 0 && (ack_buf[ack_len - 1] == '\r')) {
            ack_buf[--ack_len] = 0;
          }
          debug_OTA("MD5 reply: [%s] len=%d", ack_buf, ack_len);

          if (strcmp(ack_buf, "OK") == 0) {
            Serial1.write((uint8_t *)&write_data_ota.data, 32);
            md5_sent = true;
            debug_OTA("MD5 da gui thanh cong!");
            break;
          }
          delay(200); // Chờ U7 ổn định trước khi retry
        }
        if (!md5_sent) {
          debug_OTA("THAT BAI: U7 khong nhan SIZE_OTA MD5 sau 10 lan retry");
        }

        // Chờ U7 dò MD5, flash bộ nhớ và trả lời kết quả
        debug_OTA("Cho U7 xu ly MD5 va tra ve ket qua (Timeout 45s)...");
        long wait_t = millis();
        bool u7_success = false;
        
        uint8_t rx_buf[512];
        int rx_idx = 0;
        
        while (millis() - wait_t < 45000) {
            while (Serial1.available() && rx_idx < 500) {
                rx_buf[rx_idx++] = Serial1.read();
            }
            
            // Tìm chuỗi thô bằng vòng lặp thủ công để vượt quả hiểm cảnh NULL Bytes (0x00)
            if (rx_idx >= 13) {
                for (int i = 0; i <= rx_idx - 13; i++) {
                    if (memcmp(&rx_buf[i], "Update_comple", 13) == 0) {
                        u7_success = true;
                        debug_OTA(">> U7 thong bao: Update_comple (MD5 dung, Flash thanh cong)!");
                        goto CHECK_DONE;
                    }
                }
            }
            
            if (rx_idx >= 8) {
                for (int i = 0; i <= rx_idx - 8; i++) {
                    if (memcmp(&rx_buf[i], "MD5_fail", 8) == 0) {
                        debug_OTA(">> U7 thong bao: MD5_fail (File rong hoac bit sai lech)!");
                        goto CHECK_DONE;
                    }
                }
            }
            
            if (rx_idx >= 11) {
                for (int i = 0; i <= rx_idx - 11; i++) {
                    if (memcmp(&rx_buf[i], "Update_fail", 11) == 0) {
                        debug_OTA(">> U7 thong bao: Update_fail (Loi ghi Flash)!");
                        goto CHECK_DONE;
                    }
                }
            }
            delay(10);
        }
CHECK_DONE:

        if (!u7_success && rx_idx > 0) {
            debug_OTA("U6 Timeout/Fail. So byte nhan duoc: %d", rx_idx);
        }

        // Dọn dẹp
        sd_delete_file(OTSD_Firmware_LED);

        // Báo trạng thái THẬT sự của U7 lên Web/MQTT
        g_u7_ota_status = u7_success ? "SUCCESS" : "FAILED";
        report_UpdateOTA(u7_success);

        TimeOut.ToEUpdate(1000);
        Update_info.DW_update = false;
        otaled_flag = ota_null;
        FlagOTA = false;

        // Chỉ Reboot U6 khi bề dưới (U7) đã thực sự thành công
        if (u7_success) {
            debug_OTA("U6 reboot de dong bo voi U7...");
            delay(2000);
            ESP.restart();
        } else {
            debug_OTA("OTA That Bai! U6 xoa luong, KHONG reboot.");
            while(Serial1.available()) Serial1.read(); // Xả rác tồn đọng
        }
      }
      break;
    case ota_send_size:
      debug_OTA("send size %d", write_data_ota.size_data);
      Serial1.printf("SIZE_OTA:%d\n", write_data_ota.size_data);
      TimeOut.ToEUpdate(2000);
      otaled_flag = ota_wait_reply;
      break;

    case ota_send_data:

      debug_OTA("continue send data");
      // KHÔNG ĐƯỢC IN DỮ LIỆU NHỊ PHÂN THEO %s ! (Gây tràn RAM/Chậm chip)

      Serial1.write((uint8_t *)&write_data_ota.data, write_data_ota.size_data);

      // === Gửi thêm 2 byte CRC16 sau data để U7 verify per-chunk ===
      {
        uint16_t chunk_crc = ota_calcCRC16((uint8_t *)&write_data_ota.data, write_data_ota.size_data);
        Serial1.write((uint8_t *)&chunk_crc, 2);
      }

      TimeOut.ToEUpdate(3000);
      otaled_flag = ota_wait_reply;
      break;

    case ota_wait_reply:

      if (Serial1.available()) {
        size_t len = Serial1.available();
        uint8_t buff[len];
        String data = Serial1.readStringUntil('\n');
        data.trim(); // Xóa \r\n thừa
        debug_OTA("reply: [%s]", data.c_str());
        if (strcmp((char *)data.c_str(), "OK") == 0) {
          retry = 0; // Reset retry khi nhận phản hồi OK
          otaled_flag = ota_send_data;
        } else if (strcmp((char *)data.c_str(), "FINISH") == 0) {
          debug_OTA("continue read data");
          retry = 0;
          otaled_flag = ota_read_data;
        } else if (strcmp((char *)data.c_str(), "ERROR") == 0) {
          debug_OTA("retry send (ERROR)");
          otaled_flag = ota_send_data;
        } else if (strcmp((char *)data.c_str(), "CRC_FAIL") == 0) {
          debug_OTA("retry send (CRC_FAIL - UART noise detected)");
          otaled_flag = ota_send_data;
        } else
          debug_OTA("khong hieu: [%s]", data.c_str());

        break;
      }

      if (TimeOut.ToEExpired()) {
        TimeOut.ToEUpdate(2000);
        debug_OTA("TimeOut ota_wait_reply %d", retry);
        retry++;
        // Gửi lại SIZE_OTA khi timeout (có thể U7 chưa nhận)
        Serial1.printf("SIZE_OTA:%d\n", write_data_ota.size_data);
        if (retry > 10) {
          // xoá cờ đọc file firmware
          start = 0;
          sd_delete_file(OTSD_Firmware_LED);
          g_u7_ota_status = "FAILED"; // Cập nhật trạng thái
          Update_info.DW_update = false;
          otaled_flag = ota_null;
          FlagOTA = false;
        }
      }

      break;

    case ota_exit_data:
      FTP_Handle.calculator_md5(OTSD_Firmware_LED, Update_info.Md5);
      debug_OTA("MD5:%s", Update_info.Md5);
      strcpy((char *)&write_data_ota.data, Update_info.Md5);
      // debug_OTA("send data size %d", write_data_ota.size_data);
      Serial1.write((uint8_t *)&write_data_ota.data,
                    strlen((char *)&write_data_ota.data));
      sd_delete_file(OTSD_Firmware_LED);
      report_UpdateOTA(true);
      TimeOut.ToEUpdate(1000);
      Update_info.DW_update = false;
      otaled_flag = ota_null;
      FlagOTA = false;
      break;

    case ota_null: {
      if (TimeOut.ToEExpired()) {
        TimeOut.ToEUpdate(2000);
        debug_OTA("TimeOut ota_null %d", retry);
        retry++;
        if (retry > 5) {
          // xoá cờ đọc file firmware
          start = 0;
          sd_delete_file(OTSD_Firmware_LED); // Xóa file tránh OTA lại khi reset
          g_u7_ota_status = "FAILED";
          Update_info.DW_update = false;
          otaled_flag = ota_null;
          FlagOTA = false;
        } else {
          // === GỬI LẠI init_ota frame (U7 có thể miss lần trước) ===
          debug_OTA("Resend init_ota (retry %d)", retry);
          write_ota_t write_init;
          memset(&write_init, 0, sizeof(write_init));
          write_init.size_data = strlen("init_ota");
          memcpy(write_init.data, "init_ota", write_init.size_data);
          com_frame_t frameota;
          memset(&frameota, 0, sizeof(frameota));
          frameota.header.type = infor_ota;
          frameota.header.msk_regs.add = START_OTA.add;
          frameota.header.msk_regs.len =
              write_init.size_data + sizeof(write_init.size_data);
          memcpy(frameota.data, &write_init, frameota.header.msk_regs.len);
          frameota.header.check_crc =
              SPI_master.calcCRC(frameota.data, frameota.header.msk_regs.len);
          Serial1.write((uint8_t *)&frameota,
                        sizeof(frameota.header) + frameota.header.msk_regs.len);
        }
      }
      return;
    }

    default:
      break;
    }

    delay(1);
  }
}
