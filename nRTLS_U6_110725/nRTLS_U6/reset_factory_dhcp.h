#pragma once
#include <Arduino.h>

// Đường dẫn file cấu hình DHCP lưu trên SPIFFS
#ifndef DHCPCONFIG
#define DHCPCONFIG "/dhcpconfig.txt"
#endif

#define pin_reset_u7 15

/**
 * @brief Ghi cấu hình DHCP mặc định xuống SPIFFS theo định dạng JSON:
 *   {"DhcpMode":1,"IP":"192.168.0.100","GW":"192.168.0.1","Sn":"255.255.255.0","DNS":"8.8.8.8"}
 *
 * @param path      Đường dẫn file (mặc định: DHCPCONFIG)
 * @param autoMount Tự động SPIFFS.begin(true) nếu chưa mount (mặc định true)
 * @return true     Ghi thành công
 * @return false    Ghi thất bại
 */
bool reset_config_DHCP(const char* path = DHCPCONFIG, bool autoMount = true);

/** =========================
 *  NÚT RESET FACTORY (GPIO2)
 *  =========================
 *
 *  Sử dụng cho nút nhấn nối về GND (active-low), dùng INPUT_PULLUP.
 *  Gọi begin() một lần ở setup(), gọi task() liên tục trong loop().
 *  Khi nhấn giữ >= 10s => tự động gọi onFactoryButtonLongPress().
 */

// Khởi tạo nút (pin mặc định: 2), debounce mặc định 30ms, ngưỡng giữ 10_000ms
void factoryButtonBegin(uint8_t pin = 2, uint16_t debounceMs = 30, uint32_t longPressMs = 10000);

// Task phải gọi thường xuyên trong loop() để cập nhật chống dội & tính thời gian
void factoryButtonTask();

// Trạng thái đã chống dội (true = đang nhấn)
bool factoryButtonIsPressed();

// Thời gian đã giữ nút (ms), trả 0 nếu đang thả
uint32_t factoryButtonHoldMs();

// Hàm sẽ được gọi khi giữ nút >= longPressMs.
// Khai báo weak để bạn có thể định nghĩa lại ở sketch.
void onFactoryButtonLongPress() __attribute__((weak));
