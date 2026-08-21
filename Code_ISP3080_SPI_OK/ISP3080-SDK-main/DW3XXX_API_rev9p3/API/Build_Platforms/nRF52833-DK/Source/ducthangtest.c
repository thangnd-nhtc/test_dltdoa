//#include "ducthangtest.h"
//#include "app_timer.h"
//#include "main.h"
//#include "nrf_delay.h"
//#include <stdio.h>

//// Tham chiếu các biến và hàm từ main.c
//extern beacon_cfg_t g_beacon_cfg;
//extern uint32_t MY_DEVICE_ID;
//extern void start_config_advertising(void);

//APP_TIMER_DEF(m_ducthang_test_timer_id);

///**
// * @brief Callback xử lý sau 5 giây chờ
// */
//static void ducthang_test_handler(void *p_context) {
//  printf("\n[TEST] 5s passed! Starting BLE Config Emulation...\n");

//  // 1. Giả lập ID thiết bị

//  // 2. Giả lập một số cấu hình Beacon
//  g_beacon_cfg.val_id_new = 0x01251000; // Tag ID mới giả lập
//  g_beacon_cfg.val_motion = 1000;
//  g_beacon_cfg.val_stand = 2000;
//  g_beacon_cfg.val_id_mode = 1; // Giả lập trạng thái Mode TX

//  // Cấu hình UWB giả lập
//  g_beacon_cfg.uwb_chan = 5;
//  g_beacon_cfg.uwb_datarate = 1; // 6.8Mbps
//  g_beacon_cfg.uwb_plen = 128;

//  // 3. Kích hoạt phát cấu hình (phát trong 30s)
//  start_config_advertising();

//  printf(
//      "[TEST] Emulation started with Device ID: 0x%08lX and Tag ID: 0x%08lX\n",
//      MY_DEVICE_ID, g_beacon_cfg.val_id_new);
//}

//void ducthang_test_init(void) {
//  ret_code_t err_code;

//  printf("[TEST] Ducthang Test Initialized. Waiting 5s...\n");

//  // Tạo timer 5 giây (chạy 1 lần)
//  err_code =
//      app_timer_create(&m_ducthang_test_timer_id, APP_TIMER_MODE_SINGLE_SHOT,
//                       ducthang_test_handler);
//  APP_ERROR_CHECK(err_code);

//  // Bắt đầu đếm ngược 5 giây
//  err_code =
//      app_timer_start(m_ducthang_test_timer_id, APP_TIMER_TICKS(5000), NULL);
//  APP_ERROR_CHECK(err_code);
//}

