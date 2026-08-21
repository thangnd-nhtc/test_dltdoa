#pragma once
#include <Arduino.h>

namespace Config {

// Cấu hình EEPROM
constexpr size_t EEPROM_SIZE = 128;     // tổng dung lượng EEPROM
constexpr int EEPROM_ADDR_DEVICE_ID = 1; // địa chỉ lưu device_id (4 byte, uint32_t)

// Khởi tạo / đóng EEPROM
bool begin();
void end();

// Đọc / ghi device_id
uint32_t getDeviceID();
void setDeviceID(uint32_t id, bool commit_now = true);

// Kiểm tra device_id hợp lệ (khác 0xFFFFFFFF hoặc 0x00000000)
bool isDeviceIDValid();

} // namespace Config
