#ifndef DUCTHANGBLE_H
#define DUCTHANGBLE_H

#include "ble_beacon.h"
#include <stdint.h>

/**
 * @brief Khởi tạo các thành phần liên quan đến BLE (Stack, Timer, TX Module)
 */
void ducthang_ble_init(void);

/**
 * @brief Cập nhật dữ liệu quảng bá từ cấu hình hiện tại
 */
void ducthang_ble_update_from_cfg(void);

/**
 * @brief Bắt đầu phát gói cấu hình chi tiết (timed 30s)
 */
void start_config_advertising(void);

/**
 * @brief Kết thúc sớm quá trình phát cấu hình (gọi khi xong ACK các mảnh)
 */
void ducthang_ble_on_config_done(void);

// Callback khi phát xong 1 mảnh và nhận được ACK
void ducthang_ble_on_fragment_ack(uint16_t major, uint8_t minor);

// Callback khi Base nhận được Fragment từ Tag (Request Mode)
void ducthang_ble_on_tag_fragment_received(uint16_t major, uint8_t minor);

// Xử lý logic UWB config (tuỳ biến)
void ducthang_ble_process_uwb_config(void);

// Dừng tất cả timer BLE (gọi khi thoát BSS-TWR)
void ducthang_ble_stop_all_timers(void);

#endif // DUCTHANGBLE_H
