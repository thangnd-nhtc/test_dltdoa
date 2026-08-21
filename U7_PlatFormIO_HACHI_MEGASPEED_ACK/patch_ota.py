import re

with open("lib/Library/handle_ISP3080.cpp", "r", encoding="utf-8") as f:
    content = f.read()

new_funcs = """bool isp3080_ota_enter(uint32_t fw_size, uint32_t crc32_val) {
  uint8_t payload[8];
  // fw_size (Little Endian)
  payload[0] = (uint8_t)(fw_size & 0xFF);
  payload[1] = (uint8_t)((fw_size >> 8) & 0xFF);
  payload[2] = (uint8_t)((fw_size >> 16) & 0xFF);
  payload[3] = (uint8_t)((fw_size >> 24) & 0xFF);
  // crc32 (Little Endian)
  payload[4] = (uint8_t)(crc32_val & 0xFF);
  payload[5] = (uint8_t)((crc32_val >> 8) & 0xFF);
  payload[6] = (uint8_t)((crc32_val >> 16) & 0xFF);
  payload[7] = (uint8_t)((crc32_val >> 24) & 0xFF);

  uint8_t resp[32] = {0};
  
  // 1) Gửi lệnh CMD_OTA_ENTER = 0x20
  send_cmd_to_isp3080(0x20, payload, 8, resp, sizeof(resp), 500);

  // 2) Kéo dummy để đọc ACK từ ISP3080 (cần thời gian cho nRF52 chuẩn bị m_tx_buf)
  for (int retry = 0; retry < 10; retry++) {
    delay(5);
    if (send_cmd_to_isp3080(0x00, nullptr, 0, resp, sizeof(resp), 50)) {
      if (resp[0] == (0x20 | 0x80)) {
        if (resp[1] == 0x00) return true;
        Serial.printf("[U7_OTA_SPI] ENTER fail, mã lỗi: 0x%02X\\n", resp[1]);
        return false;
      }
    }
  }
  Serial.println("[U7_OTA_SPI] ENTER Timeout (kéo dummy 0x00 thất bại)");
  return false;
}

bool isp3080_ota_data(uint32_t offset, const uint8_t *data, uint32_t len) {
  if (len > 480) return false; // MAX_OTA_CHUNK_SIZE
  uint8_t payload[512];
  // offset (Little Endian)
  payload[0] = (uint8_t)(offset & 0xFF);
  payload[1] = (uint8_t)((offset >> 8) & 0xFF);
  payload[2] = (uint8_t)((offset >> 16) & 0xFF);
  payload[3] = (uint8_t)((offset >> 24) & 0xFF);
  // len (Little Endian)
  payload[4] = (uint8_t)(len & 0xFF);
  payload[5] = (uint8_t)((len >> 8) & 0xFF);
  payload[6] = (uint8_t)((len >> 16) & 0xFF);
  payload[7] = (uint8_t)((len >> 24) & 0xFF);

  memcpy(&payload[8], data, len);

  uint8_t resp[32] = {0};
  // CMD_OTA_DATA = 0x21
  send_cmd_to_isp3080(0x21, payload, 8 + len, resp, sizeof(resp), 1000);

  for (int retry = 0; retry < 15; retry++) {
    delay(5);
    if (send_cmd_to_isp3080(0x00, nullptr, 0, resp, sizeof(resp), 50)) {
      if (resp[0] == (0x21 | 0x80)) {
        if (resp[1] == 0x00) return true;
        Serial.printf("[U7_OTA_SPI] DATA fail offset %lu, lỗi: 0x%02X\\n", offset, resp[1]);
        return false;
      }
    }
  }
  Serial.println("[U7_OTA_SPI] DATA Timeout (kéo dummy 0x00 thất bại)");
  return false;
}

bool isp3080_ota_end(void) {
  uint8_t resp[32] = {0};
  // CMD_OTA_END = 0x22
  send_cmd_to_isp3080(0x22, nullptr, 0, resp, sizeof(resp), 2000);

  // Quá trình End có thể bao gồm chép flash / reboot trên ISP3080 nên đợi lâu hơn
  for (int retry = 0; retry < 40; retry++) {
    delay(50);
    if (send_cmd_to_isp3080(0x00, nullptr, 0, resp, sizeof(resp), 50)) {
      if (resp[0] == (0x22 | 0x80)) {
        if (resp[1] == 0x00) return true;
        Serial.printf("[U7_OTA_SPI] END fail, lỗi: 0x%02X\\n", resp[1]);
        return false;
      }
    }
  }
  Serial.println("[U7_OTA_SPI] END Timeout (kéo dummy 0x00 thất bại)");
  return false;
}

bool isp3080_ota_abort(void) {
  uint8_t resp[32] = {0};
  // CMD_OTA_ABORT = 0x23
  send_cmd_to_isp3080(0x23, nullptr, 0, resp, sizeof(resp), 500);

  for (int retry = 0; retry < 10; retry++) {
    delay(5);
    if (send_cmd_to_isp3080(0x00, nullptr, 0, resp, sizeof(resp), 50)) {
      if (resp[0] == (0x23 | 0x80)) {
        if (resp[1] == 0x00) return true;
        Serial.printf("[U7_OTA_SPI] ABORT fail, lỗi: 0x%02X\\n", resp[1]);
        return false;
      }
    }
  }
  Serial.println("[U7_OTA_SPI] ABORT Timeout (kéo dummy 0x00 thất bại)");
  return false;
}
"""

pattern = r"bool isp3080_ota_enter\(uint32_t fw_size, uint32_t crc32_val\).*?bool isp3080_ota_abort\(void\) \{.*?return false;\n\}"
content = re.sub(pattern, new_funcs, content, flags=re.DOTALL)

with open("lib/Library/handle_ISP3080.cpp", "w", encoding="utf-8") as f:
    f.write(content)
