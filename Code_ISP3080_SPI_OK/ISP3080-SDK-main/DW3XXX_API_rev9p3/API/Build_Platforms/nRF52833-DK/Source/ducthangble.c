#include "ducthangble.h"
#include "app_timer.h"
#include "main.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "rx_ble.h"
#include "tx_ble.h"
#include <stdio.h>
#include <string.h>


#define ADV_CONFIG_TIMEOUT_MS APP_TIMER_TICKS(60000)
APP_TIMER_DEF(m_adv_timeout_timer_id);

// Tham chiếu các biến toàn cục từ main.c hoặc các file khác
extern uint32_t MY_DEVICE_ID;
extern beacon_cfg_t g_beacon_cfg;
static uint32_t m_current_timeout_ms = 60000;

static tx_ble_cfg_pair_t
    m_adv_configs[64]; // Tăng từ 25 lên 64 để tránh tràn bộ nhớ
static tx_ble_custom_data_t m_base_data;

// Tham chiếu biến điều khiển TWR từ main.c
extern bool flag_send_start_twr;
extern uint32_t ss_twr_target_tag_id;

static bool m_pending_twr_mode = false;
static uint32_t m_pending_target_id = 0;

// ==== Khởi tạo BLE Stack ====
#define APP_BLE_CONN_CFG_TAG 1
static void ble_stack_init(void)
{
  ret_code_t err_code;
  err_code = nrf_sdh_enable_request();
  APP_ERROR_CHECK(err_code);
  uint32_t ram_start = 0;
  err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
  APP_ERROR_CHECK(err_code);
  err_code = nrf_sdh_ble_enable(&ram_start);
  APP_ERROR_CHECK(err_code);
}

// ==== Callback Timeout 30s ====
static void adv_timeout_handler(void *p_context)
{
  if (g_beacon_cfg.enable_bcast_twr)
  {
    // BSS-TWR mode: KHÔNG ĐƯỢC tắt BLE quảng bá!
    // Tự khởi động lại quảng bá để Tag tiếp tục nhìn thấy Base
    printf("[BLE] Global Timeout in BSS-TWR mode -> Restarting ADV\n");
    start_config_advertising(); // Phát lại BLE liên tục
    return;
  }
  printf("[BLE] Global Timeout -> Switching to RX Mode\n");
  tx_ble_stop();
  rx_ble_scan_start();
}

void ducthang_ble_on_config_done(void)
{
  // printf("[BLE] Fragmentation FULL SUCCESS -> Mode: %s\n",
  //        m_pending_twr_mode ? "DS-TWR" : "Normal");

  // Nếu BSS-TWR đang bật, KHÔNG tắt BLE - phải tiếp tục quảng bá!
  if (g_beacon_cfg.enable_bcast_twr)
  {
    printf("[BLE] Config done in BSS-TWR mode -> keep advertising\n");
    // Khởi động lại quảng bá BSS-TWR
    start_config_advertising();
    return;
  }

  app_timer_stop(m_adv_timeout_timer_id);
  tx_ble_stop();
  rx_ble_scan_start();

  // CHỈ KÍCH HOẠT KHI TẤT CẢ MẢNH ĐÃ XONG (Dành cho Mode 5 DucThang - Unicast)
  if (m_pending_twr_mode && !g_beacon_cfg.enable_bcast_twr)
  {
    extern bool flag_start_ducthang_twr;
    ss_twr_target_tag_id = m_pending_target_id;
    flag_start_ducthang_twr = true; // Kích hoạt flag RIÊNG
    m_pending_twr_mode = false;
    printf("[BLE] >>> All fragments ACKed. Starting SS-TWR (DucThang) to Tag: "
           "%lu\n",
           ss_twr_target_tag_id);
  }
  else if (m_pending_twr_mode && g_beacon_cfg.enable_bcast_twr)
  {
    m_pending_twr_mode = false;
  }

  // Set flag to save flash
  extern bool flag_save_config_pending;
  flag_save_config_pending = true;
}

void ducthang_ble_init(void)
{
  ble_stack_init();
  tx_ble_init();
  ret_code_t err_code = app_timer_create(
      &m_adv_timeout_timer_id, APP_TIMER_MODE_SINGLE_SHOT, adv_timeout_handler);
  APP_ERROR_CHECK(err_code);
  rx_ble_init();
  rx_ble_scan_start();
}

void ducthang_ble_update_from_cfg(void)
{
  // 1. Chuẩn bị dữ liệu nền (Base Data)
  memset(&m_base_data, 0, sizeof(m_base_data));
  m_base_data.encryption_key[0] = 0x14;
  m_base_data.encryption_key[1] = 0x05;
  m_base_data.encryption_key[2] = 0x02;
  m_base_data.encryption_key[3] = 0x20;
  m_base_data.encryption_key[4] = 0x08;
  m_base_data.encryption_key[5] = 0x03;

  // Convert IDs: MY_DEVICE_ID (uint32) -> BCD Array -> id_base
  // Ví dụ: 1025100023 -> split: 23, 00, 10, 25, 10 -> {0x10, 0x25, 0x10, 0x00,
  // 0x23}
  uint32_t temp_id = MY_DEVICE_ID;
  uint8_t temp_bcd[5];

  for (int i = 4; i >= 0; i--)
  {
    uint8_t two_digits = temp_id % 100;
    temp_id /= 100;
    temp_bcd[i] = ((two_digits / 10) << 4) | (two_digits % 10);
  }

  // Copy mảng BCD vào id_base và lưu biến raw toàn cục
  extern uint8_t my_base_id_raw[5];
  memcpy(my_base_id_raw, temp_bcd, 5);
  memcpy(m_base_data.id_base, temp_bcd, 5);

  // Xác định Target ID (dùng MAC address nếu nó có byte khác 0, ngược lại dùng
  // val_id_last)
  bool use_mac_as_target = false;
  for (int i = 0; i < 5; i++)
  {
    if (g_beacon_cfg.mac_address[i] != 0)
    {
      use_mac_as_target = true;
      break;
    }
  }

  // if (use_mac_as_target && (g_beacon_cfg.val_id_change == 1)) {
  if (g_beacon_cfg.enable_bcast_twr)
  {
    memset(m_base_data.id_tag, 0xFF, 5); // Broadcast ID for TWR
  }
  else if (use_mac_as_target)
  {
    memcpy(m_base_data.id_tag, g_beacon_cfg.mac_address, 5);
  }
  else
  {
    memcpy(m_base_data.id_tag, g_beacon_cfg.val_id_last, 5);
  }

  m_base_data.measured_rssi = (int8_t)0xC3; // -61 dBm

  // 2. Điền danh sách cấu hình (Pairs)
  uint8_t count = 0;

  if (g_beacon_cfg.val_request != 0)
  {
    // ---- TRƯỜNG HỢP REQUEST: Chỉ phát mảnh Request, không phát gì khác ----
    switch (g_beacon_cfg.val_request)
    {
    case 1:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_REQUEST_TIMER, 0xFF};
      break;
    case 2:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_REQUEST_CONFIG, 0xFF};
      break;
    case 3:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_REQUEST_STATE, 0xFF};
      break;
    case 4:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_REQUEST_ID_TAG, 0xFF};
      break;
    default:
      break;
    }
  }
  else
  {
    // ---- TRƯỜNG HỢP CẤU HÌNH BÌNH THƯỜNG: Phát các mảnh như cũ ----
    if (g_beacon_cfg.val_motion && count < 60)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_TIMER_SEND_TAG_MOTION, (uint8_t)g_beacon_cfg.val_motion};
    if (g_beacon_cfg.val_stand)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_TIMER_SEND_TAG_STAND, (uint8_t)g_beacon_cfg.val_stand};
    if (g_beacon_cfg.val_sleep1)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_TIMER_SEND_TAG_SLEEP_MODE_1, (uint8_t)g_beacon_cfg.val_sleep1};
    if (g_beacon_cfg.val_sleep2)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_TIMER_SEND_TAG_SLEEP_MODE_2, (uint8_t)g_beacon_cfg.val_sleep2};
    if (g_beacon_cfg.val_sleep3)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_TIMER_SEND_TAG_SLEEP_MODE_3, (uint8_t)g_beacon_cfg.val_sleep3};

    if (g_beacon_cfg.val_mode1)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_TIMER_SLEEP_MODE_1, (uint8_t)g_beacon_cfg.val_mode1};
    if (g_beacon_cfg.val_mode2)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_TIMER_SLEEP_MODE_2, (uint8_t)g_beacon_cfg.val_mode2};
    if (g_beacon_cfg.val_mode3)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_TIMER_SLEEP_MODE_3, (uint8_t)g_beacon_cfg.val_mode3};

    if (g_beacon_cfg.val_batt_default)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_BATT_UPDATE_DEFAULT, (uint8_t)g_beacon_cfg.val_batt_default};
    if (g_beacon_cfg.val_batt_high)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_BATT_UPDATE_HIGH, (uint8_t)g_beacon_cfg.val_batt_high};
    if (g_beacon_cfg.val_batt_inc)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_BATT_INCREASE, (uint8_t)g_beacon_cfg.val_batt_inc};
    if (g_beacon_cfg.val_batt_dec)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_BATT_DECREASE, (uint8_t)g_beacon_cfg.val_batt_dec};

    // --- UWB Config (0x10) ---
    if (g_beacon_cfg.uwb_chan > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_CHAN, (uint8_t)(g_beacon_cfg.uwb_chan - 1)};
    if (g_beacon_cfg.uwb_plen > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_PLEN, (uint8_t)(g_beacon_cfg.uwb_plen - 1)};
    if (g_beacon_cfg.uwb_pac > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_PAC, (uint8_t)(g_beacon_cfg.uwb_pac - 1)};
    if (g_beacon_cfg.uwb_txcode > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_TXCODE, (uint8_t)g_beacon_cfg.uwb_txcode};
    if (g_beacon_cfg.uwb_rxcode > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_RXCODE, (uint8_t)g_beacon_cfg.uwb_rxcode};
    if (g_beacon_cfg.uwb_sfdtype > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_SFDTYPE, (uint8_t)g_beacon_cfg.uwb_sfdtype};
    if (g_beacon_cfg.uwb_datarate > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_DATARATE, (uint8_t)(g_beacon_cfg.uwb_datarate - 1)};
    if (g_beacon_cfg.uwb_phrmode > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_PHRMODE, (uint8_t)(g_beacon_cfg.uwb_phrmode - 1)};
    if (g_beacon_cfg.uwb_phrrate > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_PHRRATE, (uint8_t)(g_beacon_cfg.uwb_phrrate - 1)};
    if (g_beacon_cfg.uwb_sfdto > 0)
    {
      uint8_t data0 = (g_beacon_cfg.uwb_sfdto >> 7) & 0x7F;
      uint8_t data1 = g_beacon_cfg.uwb_sfdto & 0x7F;
      m_adv_configs[count++] =
          (tx_ble_cfg_pair_t){MAJOR_CONFIG_UWB_SFDTO, (uint8_t)(data0 | 0x00)};
      m_adv_configs[count++] =
          (tx_ble_cfg_pair_t){MAJOR_CONFIG_UWB_SFDTO, (uint8_t)(data1 | 0x80)};
    }
    if (g_beacon_cfg.uwb_stsmode > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_STSMODE, (uint8_t)(g_beacon_cfg.uwb_stsmode - 1)};
    if (g_beacon_cfg.uwb_stslen > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_STSLEN, (uint8_t)(g_beacon_cfg.uwb_stslen - 1)};
    if (g_beacon_cfg.uwb_pdoa > 0)
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_CONFIG_UWB_PDOA, (uint8_t)g_beacon_cfg.uwb_pdoa};

    // --- State Mapping ---
    switch (g_beacon_cfg.val_id_mode)
    {
    case 0xFE:
      // Không khai báo val_id_mode trong gói cấu hình JSON => Bỏ qua không đổi STATE
      break;
    case 0:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_STATE_DEFAULT, 0xFF};
      break;
    case 1:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_STATE_TX, 0xFF};
      break;
    case 2:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_STATE_OFF_UWB, 0xFF};
      break;
    case 3:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_STATE_SOS, 0xFF};
      break;
    case 4:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_STATE_IDENTIFY, 0xFF};
      break;
    case 5:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_STATE_TWR, 0xFF};
      break;
    case 6:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_STATE_RESET, 0xFF};
      break;
    case 7:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_STATE_AIRPLAN, 0xFF};
      break;
    case 8:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_STATE_MOTION, 0xFF};
      break;
    default:
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_STATE_DEFAULT, 0xFF};
      break;
    }

    // --- ID Change ---
    if (g_beacon_cfg.val_id_change == 1)
    {
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_ID_CHANGE_BYTE1, (uint8_t)g_beacon_cfg.val_id_new[0]};
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_ID_CHANGE_BYTE2, (uint8_t)g_beacon_cfg.val_id_new[1]};
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_ID_CHANGE_BYTE3, (uint8_t)g_beacon_cfg.val_id_new[2]};
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_ID_CHANGE_BYTE4, (uint8_t)g_beacon_cfg.val_id_new[3]};
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){
          MAJOR_ID_CHANGE_BYTE5, (uint8_t)g_beacon_cfg.val_id_new[4]};
    }

    // --- CHARGE_TX Setting (0x40) ---
    if (g_beacon_cfg.charge_tx == 1)
    {
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_SETTING_CHARGE_TX, 0x00}; // OFF
    }
    else if (g_beacon_cfg.charge_tx == 2)
    {
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_SETTING_CHARGE_TX, 0xFF}; // ON
    }

    // --- OTA Setting (0x4001) ---
    if (g_beacon_cfg.ota_enable == 1)
    {
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_SETTING_OTA, 0xFF}; // ENABLE OTA
    }

    // --- SYS_CONFIG_DEFAULT Setting (0x4002) ---
    if (g_beacon_cfg.sys_config_default == 1)
    {
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_SETTING_SYS_CONFIG_DEFAULT, 0xFF}; // Reset to default
    }

    // --- SLEEP Setting (0x4003) ---
    if (g_beacon_cfg.sleep_enable == 1)
    {
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_SETTING_SLEEP, 0x00}; // DISABLE SLEEP
    }
    else if (g_beacon_cfg.sleep_enable == 2)
    {
      m_adv_configs[count++] = (tx_ble_cfg_pair_t){MAJOR_SETTING_SLEEP, 0xFF}; // ENABLE SLEEP
    }
  }

  if (count == 0)
    m_adv_configs[count++] = (tx_ble_cfg_pair_t){0x0000, 0x0000};

  // Lưu ID tag raw 5 byte toàn cục để sử dụng cho so sánh gói ACK/Nhận từ Tag
  extern uint8_t current_tag_id_raw[5];
  memcpy(current_tag_id_raw, m_base_data.id_tag, 5);

  // Lưu trạng thái chờ đo khoảng cách TRƯỚC KHI xóa g_beacon_cfg
  if (g_beacon_cfg.val_id_mode == 5 && !g_beacon_cfg.enable_bcast_twr)
  {
    m_pending_twr_mode = true;

    uint32_t id_converted = 0;
    for (int i = 0; i < 5; i++)
    {
      uint8_t b = g_beacon_cfg.val_id_last[i];
      if (b != 0xFF)
      {
        id_converted = id_converted * 100 + ((b >> 4) * 10) + (b & 0x0F);
      }
    }
    m_pending_target_id = id_converted;
  }
  else if (g_beacon_cfg.val_id_mode == 5 && g_beacon_cfg.enable_bcast_twr)
  {
    m_pending_twr_mode = false; // BSS-TWR không dùng logic "đợi ACK đổi config"
  }
  else
  {
    m_pending_twr_mode = false;
  }

  bool is_broadcast = false;
  if (g_beacon_cfg.enable_bcast_twr)
  {
    is_broadcast = true; // BSS-TWR luôn là broadcast
  }
  else if (g_beacon_cfg.broadcast == 1)
  {
    is_broadcast = true;
  }
  else
  {
    bool all_ff = true;
    for (int i = 0; i < 5; i++)
    {
      if (g_beacon_cfg.val_id_last[i] != 0xFF)
      {
        all_ff = false;
        break;
      }
    }
    if (all_ff)
      is_broadcast = true;
  }

  if (is_broadcast)
  {
    uint8_t filtered_count = 0;
    for (int i = 0; i < count; i++)
    {
      uint8_t high_byte = m_adv_configs[i].major >> 8;
      // Broadcast mode applies to Timer(0x00), Sleep(0x01), Batt(0x02), Config(0x10), State(0x20), Setting(0x40)
      if (high_byte == 0x00 || high_byte == 0x01 || high_byte == 0x02 || high_byte == 0x10 || high_byte == 0x20 || high_byte == 0x40)
      {
        m_adv_configs[filtered_count++] = m_adv_configs[i];
      }
    }
    count = filtered_count;
    if (count > 0 && !g_beacon_cfg.enable_bcast_twr)
    {
      m_current_timeout_ms = count * 10000 + 1000; // 10s per fragment + 1s buffer
      tx_ble_start_broadcast_advertising(&m_base_data, m_adv_configs, count,
                                         10000);
    }
    else if (g_beacon_cfg.enable_bcast_twr)
    {
      // Nếu enable_bcast_twr == 1, ta ưu tiên phát BSS-TWR điểm danh thay vì broadcast config.
      // Ta sẽ phát gói dummy state (hoặc packet TWR) để Tag biết
      m_adv_configs[0] = (tx_ble_cfg_pair_t){MAJOR_STATE_TWR, 0xFF};
      count = 1;
      m_current_timeout_ms = 0;                                                  // 0 = timeout vô hạn
      tx_ble_start_broadcast_advertising(&m_base_data, m_adv_configs, count, 0); // 0 = no timeout per frg
    }
  }
  else
  {
    m_current_timeout_ms = 60000; // 60s max for unicast
    tx_ble_start_fragmented_advertising(&m_base_data, m_adv_configs, count, 50);
  }

  // Lưu mode, request và cờ apply trước khi xóa config
  uint8_t saved_mode = g_beacon_cfg.val_id_mode;
  uint8_t saved_request = g_beacon_cfg.val_request;
  uint8_t saved_apply = g_beacon_cfg.apply_config_to_base;
  uint8_t saved_enable_bcast_twr = g_beacon_cfg.enable_bcast_twr;

  memset(&g_beacon_cfg, 0, sizeof(beacon_cfg_t));

  // Khôi phục lại mode nếu đang ở SS-TWR (mode 5) hoặc đang Request để main
  // loop/request handler tiếp tục chạy
  if (saved_mode == 5)
  {
    g_beacon_cfg.val_id_mode = 5;
  }
  g_beacon_cfg.val_request = saved_request;
  g_beacon_cfg.apply_config_to_base = saved_apply;
  g_beacon_cfg.enable_bcast_twr = saved_enable_bcast_twr;
}

void ducthang_ble_on_fragment_ack(uint16_t major, uint8_t minor)
{
  // Hàm này để trống hoặc dùng cho logic log tiến độ nếu cần
  // (Đã chuyển logic trigger sang ducthang_ble_on_config_done)
}

void start_config_advertising(void)
{
  if (!g_beacon_cfg.enable_bcast_twr)
  {
    rx_ble_scan_stop(); // Chỉ dừng scan khi KHÔNG phải BSS-TWR
  }
  ducthang_ble_update_from_cfg();
  app_timer_stop(m_adv_timeout_timer_id);
  if (g_beacon_cfg.enable_bcast_twr)
  {
    // BSS-TWR: Bắt buộc start timer 30s để tự phục hồi ADV nếu bị tắt
    app_timer_start(m_adv_timeout_timer_id, APP_TIMER_TICKS(30000), NULL);
    // Đảm bảo scan luôn chạy song song với advertising
    rx_ble_scan_start();
  }
  else if (m_current_timeout_ms > 0)
  {
    app_timer_start(m_adv_timeout_timer_id, APP_TIMER_TICKS(m_current_timeout_ms), NULL);
  }
}

void ducthang_ble_stop_all_timers(void)
{
  app_timer_stop(m_adv_timeout_timer_id);
  tx_ble_stop();
  printf("[BLE] All timers stopped (BSS-TWR exit)\n");
}
