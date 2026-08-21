#include "tx_ble.h"
#include "app_error.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_err.h"
#include "ble_gap.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "main.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "Source/ducthangble.h"
#include "Source/rx_ble.h"
#include "app_timer.h" // Thêm thư viện Timer
#include "nrf_delay.h"
#include "request.h"
#include <stdio.h>

// Cấu hình Advertising
#define APP_BLE_CONN_CFG_TAG 1
// Advertising interval: 100ms. Unit is 0.625ms. => 100 / 0.625 = 160
#define NON_CONNECTABLE_ADV_INTERVAL 160
#define APP_BEACON_INFO_LENGTH \
  0x17                                // Tổng độ dài Payload Manufacturer Data (23 bytes)
#define APP_ADV_DATA_LENGTH 0x15      // Độ dài dữ liệu thực tế iBeacon (21 bytes)
#define APP_DEVICE_TYPE 0x02          // iBeacon Type
#define APP_COMPANY_IDENTIFIER 0x0059 // Nordic Semiconductor

// Handle cho Advertising set (SoftDevice S140 hỗ trợ nhiều set, ta dùng 1 cái
// mặc định)
static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;

static ble_gap_adv_params_t m_adv_params;
static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
static ble_gap_adv_data_t m_adv_data = {
    .adv_data = {.p_data = m_enc_advdata, .len = BLE_GAP_ADV_SET_DATA_SIZE_MAX},
    .scan_rsp_data = {.p_data = NULL, .len = 0}};

static uint8_t
    m_beacon_info[APP_BEACON_INFO_LENGTH]; // Bộ đệm chứa dữ liệu Beacon

// ==== Variables for Fragmented Advertising ====
APP_TIMER_DEF(m_adv_update_timer);          // Timer luân chuyển data
static tx_ble_custom_data_t m_base_data;    // Dữ liệu nền
static tx_ble_cfg_pair_t *m_p_items = NULL; // Con trỏ tới mảng dữ liệu
static uint8_t m_item_count = 0;            // Tổng số item
static uint8_t m_current_idx = 0;           // Index hiện tại
static bool m_waiting_for_ack = false;      // Cờ đang chờ ACK
static bool m_is_broadcast_mode = false;    // Cờ Broadcast
uint64_t fragment_status = 0;               // Trạng thái ACK của các mảnh

// Forward declaration
static uint8_t get_fragment_bit_index(uint16_t major);

/**
 * @brief Hàm tính CRC8 cho 1 byte dữ liệu (Polynomial 0x07)
 */
static uint8_t calculate_crc8(uint8_t data)
{
  uint8_t crc = 0x00;
  crc ^= data;

  for (uint8_t j = 0; j < 8; j++)
  {
    if (crc & 0x80)
    {
      crc = (crc << 1) ^ 0x07;
    }
    else
    {
      crc <<= 1;
    }
  }

  return crc;
}

/**
 * @brief Hàm xử lý Timer timeout -> Cập nhật gói quảng bá kế tiếp
 */
static void adv_update_timeout_handler(void *p_context)
{
  UNUSED_PARAMETER(p_context);

  if (m_is_broadcast_mode)
  {
    if (m_current_idx >= m_item_count)
    {
      // Kiểm tra xem có phải BSS-TWR không - nếu có thì loop lại thay vì dừng
      extern beacon_cfg_t g_beacon_cfg;
      if (g_beacon_cfg.enable_bcast_twr)
      {
        // BSS-TWR: Reset index về 0 để phát lại gói mồi liên tục
        m_current_idx = 0;
        // Không return, tiếp tục xuống dưới để phát lại fragment 0
      }
      else
      {
        // Đã broadcast xong tất cả fragment (mode thường)
        app_timer_stop(m_adv_update_timer);
        if (m_adv_handle != BLE_GAP_ADV_SET_HANDLE_NOT_SET)
        {
          sd_ble_gap_adv_stop(m_adv_handle);
        }
        m_is_broadcast_mode = false;

        // --- BÁO CÁO HOÀN THÀNH BROADCAST QUA SPI ---
        printf("[BLE] Broadcast COMPLETE (%d fragments, 30s each)\n",
               m_item_count);
        fragment_status |= (1ULL << 63); // SET FINISH FLAG
        spi_send_ack_status_update();    // Gửi báo cáo qua SPI để U7 biết là xong

        // BÁO VỀ DUCTHANGBLE ĐỂ DỪNG TIMER GLOBAL VÀ CHUYỂN MODE
        ducthang_ble_on_config_done();
        return;
      }
    }

    tx_ble_cfg_pair_t current_cfg = m_p_items[m_current_idx];
    m_base_data.struct_id = (uint8_t)(current_cfg.major >> 8);
    m_base_data.struct_offset = (uint8_t)(current_cfg.major & 0xFF);
    m_base_data.value_to_change = (uint8_t)current_cfg.minor;
    m_base_data.crc_minor = calculate_crc8(m_base_data.value_to_change);

    // Nếu là chế độ Broadcast TWR (Điểm danh), ta gán cứng destination ID =
    // FFFFFFFFFF
    if (current_cfg.major == MAJOR_STATE_TWR)
    {
      memset(m_base_data.id_tag, 0xFF, 5);
      // Đảm bảo ID Base cũng được cập nhật đúng để Tag biết Base nào đang gọi
      extern uint8_t my_base_id_raw[5];
      memcpy(m_base_data.id_base, my_base_id_raw, 5);
    }

    m_waiting_for_ack = false;
    // Phát liên tục (0 sự kiện)
    tx_ble_update_data_and_start(&m_base_data, 0, 160);

    // --- ACK tung manh: set bit cho fragment vua phat va bao SPI ---
    uint16_t major_sent = current_cfg.major;
    uint8_t bit_idx = get_fragment_bit_index(major_sent);
    if (bit_idx < 64)
    {
      fragment_status |= (1ULL << bit_idx);
    }
    // Kiem tra day la manh cuoi cung khong
    if (m_current_idx + 1 >= m_item_count)
    {
      fragment_status |= (1ULL << 63); // SET FINISH FLAG cho manh cuoi
    }
    printf("[BLE] Broadcast fragment %d/%d sent (bit %d), status=0x%08X%08X\n",
           m_current_idx + 1, m_item_count, bit_idx,
           (uint32_t)(fragment_status >> 32), (uint32_t)(fragment_status & 0xFFFFFFFF));
    spi_send_ack_status_update(); // Bao SPI ngay sau moi manh

    // Tăng index để 30s sau xử lý fragment tiếp theo
    m_current_idx++;
    return;
  }

  // --- Logic của Unicast (Chờ ACK) ---
  if (m_p_items == NULL || m_item_count == 0)
    return;
  // 1. CHUẨN BỊ MẢNH HIỆN TẠI
  tx_ble_cfg_pair_t current_cfg = m_p_items[m_current_idx];
  m_base_data.struct_id = (uint8_t)(current_cfg.major >> 8);
  m_base_data.struct_offset = (uint8_t)(current_cfg.major & 0xFF);
  m_base_data.value_to_change = (uint8_t)current_cfg.minor;
  m_base_data.crc_minor = calculate_crc8(m_base_data.value_to_change);
  // 2. PHÁT GÓI (Asynchronous)
  // Scan vẫn đang chạy ngầm, Radio sẽ phát 1 gói rồi tự quay lại nhận
  m_waiting_for_ack = true;
  // Khi Base phát mảnh (Fragment) cho Tag: phát 1 gói rồi đợi ACK (mặc định
  // interval 100ms)
  tx_ble_update_data_and_start(&m_base_data, 1, 160);

  // Timer (interval_ms) sẽ gọi lại hàm này để "Retry" phát mảnh này nếu chưa
  // nhận được ACK
}

/**
 * @brief Khởi tạo module Advertising
 */
static void advertising_init(void)
{
  uint32_t err_code;

  // Tạo Timer
  err_code = app_timer_create(&m_adv_update_timer, APP_TIMER_MODE_REPEATED,
                              adv_update_timeout_handler);
  APP_ERROR_CHECK(err_code);

  memset(&m_adv_params, 0, sizeof(m_adv_params));

  m_adv_params.properties.type =
      BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
  m_adv_params.p_peer_addr = NULL; // Undirected
  m_adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;
  m_adv_params.interval = NON_CONNECTABLE_ADV_INTERVAL;
  m_adv_params.duration = 0; // Chạy vô hạn

  // Phát 1 gói rồi dừng để nhận ACK ngay
  m_adv_params.max_adv_evts = 1;

  m_adv_params.primary_phy = BLE_GAP_PHY_1MBPS;
  m_adv_params.secondary_phy = BLE_GAP_PHY_1MBPS;
}

void tx_ble_start_fragmented_advertising(tx_ble_custom_data_t *p_base_data,
                                         tx_ble_cfg_pair_t *p_items,
                                         uint8_t item_count,
                                         uint32_t interval_ms)
{
  if (p_items == NULL || item_count == 0)
    return;

  // printf("[BLE] Fragmentation Start: %d items, interval %lu ms\n",
  // item_count,
  //        interval_ms);

  // 1. Lưu lại cấu hình
  memcpy(&m_base_data, p_base_data, sizeof(tx_ble_custom_data_t));
  m_p_items = p_items;
  m_item_count = item_count;
  m_current_idx = 0;
  m_waiting_for_ack = false;
  m_is_broadcast_mode =
      false;           // <-- FIX BUG: Phải reset flag m_is_broadcast_mode ở đây!
  fragment_status = 0; // Reset trạng thái

  // Bật Scan xuyên suốt để không bỏ lỡ ACK nhanh
  rx_ble_scan_start();

  // 2. Chạy Timer
  uint32_t err_code;
  err_code = app_timer_stop(m_adv_update_timer);

  // Cập nhật lại chu kỳ Timer nếu cần (User truyền vào)
  err_code =
      app_timer_start(m_adv_update_timer, APP_TIMER_TICKS(interval_ms), NULL);
  APP_ERROR_CHECK(err_code);

  // Gọi phát gói đầu tiên
  adv_update_timeout_handler(NULL);
}

void tx_ble_start_broadcast_advertising(tx_ble_custom_data_t *p_base_data,
                                        tx_ble_cfg_pair_t *p_items,
                                        uint8_t item_count,
                                        uint32_t interval_ms)
{
  if (p_items == NULL || item_count == 0)
    return;

  // 1. Lưu lại cấu hình
  memcpy(&m_base_data, p_base_data, sizeof(tx_ble_custom_data_t));
  m_p_items = p_items;
  m_item_count = item_count;
  m_current_idx = 0;
  m_waiting_for_ack = false;
  m_is_broadcast_mode = true;
  fragment_status = 0; // Reset trạng thái

  // 2. Bắt đầu Timer (interval = 30s)
  uint32_t err_code;
  err_code = app_timer_stop(m_adv_update_timer);

  if (interval_ms > 0)
  {
    err_code =
        app_timer_start(m_adv_update_timer, APP_TIMER_TICKS(interval_ms), NULL);
    APP_ERROR_CHECK(err_code);
  }

  // Gọi ngay để phát fragment đầu tiên (m_current_idx = 0)
  adv_update_timeout_handler(NULL);
}

/**
 * @brief Hàm khởi tạo công khai
 */
void tx_ble_init(void) { advertising_init(); }

/**
 * @brief Helper để ghi 32-bit Big Endian (vào vùng UUID)
 */
static void uint32_to_big_endian(uint32_t value, uint8_t *p_buf)
{
  p_buf[0] = (uint8_t)((value >> 24) & 0xFF);
  p_buf[1] = (uint8_t)((value >> 16) & 0xFF);
  p_buf[2] = (uint8_t)((value >> 8) & 0xFF);
  p_buf[3] = (uint8_t)(value & 0xFF);
}

/**
 * @brief Cập nhật dữ liệu và bắt đầu phát
 */
void tx_ble_update_data_and_start(tx_ble_custom_data_t *p_data,
                                  uint8_t max_evts, uint16_t interval_0_625ms)
{
  uint32_t err_code;

  m_adv_params.max_adv_evts = max_evts;
  m_adv_params.interval = interval_0_625ms;

  if (m_adv_handle != BLE_GAP_ADV_SET_HANDLE_NOT_SET)
  {
    sd_ble_gap_adv_stop(m_adv_handle);
  }
  // Byte 0: Device Type (0x02)
  m_beacon_info[0] = APP_DEVICE_TYPE;
  // Byte 1: Length (0x15 = 21 bytes còn lại)
  m_beacon_info[1] = APP_ADV_DATA_LENGTH;
  memcpy(&m_beacon_info[2], p_data->encryption_key, 6);
  memcpy(&m_beacon_info[8], p_data->id_base, 5);
  memcpy(&m_beacon_info[13], p_data->id_tag, 5);
  // printf("[BLE][ADV DATA] major=%02X%02X minor=%02X%02X id_tag=%02X%02X%02X%02X%02X\n",
  //        p_data->struct_id,
  //        p_data->struct_offset,
  //        p_data->crc_minor,
  //        p_data->value_to_change,
  //        p_data->id_tag[0],
  //        p_data->id_tag[1],
  //        p_data->id_tag[2],
  //        p_data->id_tag[3],
  //        p_data->id_tag[4]);

  m_beacon_info[18] = p_data->struct_id;
  m_beacon_info[19] = p_data->struct_offset;

  m_beacon_info[20] = p_data->crc_minor;
  m_beacon_info[21] = p_data->value_to_change;

  m_beacon_info[22] = (uint8_t)p_data->measured_rssi;

  // 3. Encode dữ liệu vào struct của Advertising
  ble_advdata_manuf_data_t manuf_data;
  manuf_data.company_identifier = APP_COMPANY_IDENTIFIER;
  manuf_data.data.p_data = m_beacon_info;
  manuf_data.data.size = APP_BEACON_INFO_LENGTH;

  ble_advdata_t advdata;
  memset(&advdata, 0, sizeof(advdata));
  advdata.name_type = BLE_ADVDATA_NO_NAME;
  advdata.flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
  advdata.p_manuf_specific_data = &manuf_data;

  // Reset length before encode
  m_adv_data.adv_data.len = BLE_GAP_ADV_SET_DATA_SIZE_MAX;
  err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data,
                                &m_adv_data.adv_data.len);
  if (err_code != NRF_SUCCESS && err_code != NRF_ERROR_INVALID_STATE)
  {
    // printf("[BLE] Encode error: %lu\n", err_code);
    return;
  }

  err_code =
      sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
  if (err_code != NRF_SUCCESS && err_code != NRF_ERROR_INVALID_STATE)
  {
    // printf("[BLE] Config error: %lu\n", err_code);
    return;
  }

  // 5. Start Advertising
  err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
  if (err_code != NRF_SUCCESS && err_code != NRF_ERROR_INVALID_STATE)
  {
    // printf("[BLE] Start error: %lu\n", err_code);
  }
}

void tx_ble_stop(void)
{
  // Dừng Timer xoay vòng mảnh nếu đang chạy
  app_timer_stop(m_adv_update_timer);
  rx_ble_scan_stop();
  m_waiting_for_ack = false;

  // Dừng quảng bá GAP
  if (m_adv_handle != BLE_GAP_ADV_SET_HANDLE_NOT_SET)
  {
    sd_ble_gap_adv_stop(m_adv_handle);
  }
}

static uint8_t get_fragment_bit_index(uint16_t major)
{
  switch (major)
  {
  case MAJOR_TIMER_SEND_TAG_MOTION:
    return 0;
  case MAJOR_TIMER_SEND_TAG_STAND:
    return 1;
  case MAJOR_TIMER_SEND_TAG_SLEEP_MODE_1:
    return 2;
  case MAJOR_TIMER_SEND_TAG_SLEEP_MODE_2:
    return 3;
  case MAJOR_TIMER_SEND_TAG_SLEEP_MODE_3:
    return 4;
  case MAJOR_TIMER_SLEEP_MODE_1:
    return 5;
  case MAJOR_TIMER_SLEEP_MODE_2:
    return 6;
  case MAJOR_TIMER_SLEEP_MODE_3:
    return 7;
  case MAJOR_BATT_UPDATE_DEFAULT:
    return 8;
  case MAJOR_BATT_UPDATE_HIGH:
    return 9;
  case MAJOR_BATT_INCREASE:
    return 10;
  case MAJOR_BATT_DECREASE:
    return 11;
  case MAJOR_CONFIG_UWB_CHAN:
    return 12;
  case MAJOR_CONFIG_UWB_PLEN:
    return 13;
  case MAJOR_CONFIG_UWB_PAC:
    return 14;
  case MAJOR_CONFIG_UWB_TXCODE:
    return 15;
  case MAJOR_CONFIG_UWB_RXCODE:
    return 16;
  case MAJOR_CONFIG_UWB_SFDTYPE:
    return 17;
  case MAJOR_CONFIG_UWB_DATARATE:
    return 18;
  case MAJOR_CONFIG_UWB_PHRMODE:
    return 19;
  case MAJOR_CONFIG_UWB_PHRRATE:
    return 20;
  case MAJOR_CONFIG_UWB_SFDTO:
    return 21;
  case MAJOR_CONFIG_UWB_STSMODE:
    return 22;
  case MAJOR_CONFIG_UWB_STSLEN:
    return 23;
  case MAJOR_CONFIG_UWB_PDOA:
    return 24;
  case MAJOR_STATE_DEFAULT:
    return 25;
  case MAJOR_STATE_TX:
    return 26;
  case MAJOR_STATE_OFF_UWB:
    return 27;
  case MAJOR_STATE_SOS:
    return 28;
  case MAJOR_STATE_IDENTIFY:
    return 29;
  case MAJOR_STATE_TWR:
    return 30;
  case MAJOR_STATE_RESET:
    return 31;
  case MAJOR_STATE_AIRPLAN:
    return 32;
  case MAJOR_STATE_MOTION:
    return 53;
  case MAJOR_REQUEST_TIMER:
    return 33;
  case MAJOR_REQUEST_CONFIG:
    return 34;
  case MAJOR_REQUEST_STATE:
    return 35;
  case MAJOR_REQUEST_ID_TAG:
    return 36;
  case MAJOR_ID_CHANGE_FLAG:
    return 37;
  case MAJOR_ID_CHANGE_BYTE1:
    return 38;
  case MAJOR_ID_CHANGE_BYTE2:
    return 39;
  case MAJOR_ID_CHANGE_BYTE3:
    return 40;
  case MAJOR_ID_CHANGE_BYTE4:
    return 41;
  case MAJOR_ID_CHANGE_BYTE5:
    return 42;
  case MAJOR_ID_LAST_BYTE1:
    return 43;
  case MAJOR_ID_LAST_BYTE2:
    return 44;
  case MAJOR_ID_LAST_BYTE3:
    return 45;
  case MAJOR_ID_LAST_BYTE4:
    return 46;
  case MAJOR_ID_LAST_BYTE5:
    return 47;
  case MAJOR_MAC_ADDRESS_BYTE1:
    return 48;
  case MAJOR_MAC_ADDRESS_BYTE2:
    return 49;
  case MAJOR_MAC_ADDRESS_BYTE3:
    return 50;
  case MAJOR_MAC_ADDRESS_BYTE4:
    return 51;
  case MAJOR_MAC_ADDRESS_BYTE5:
    return 52;
  case MAJOR_SETTING_CHARGE_TX:
    return 54;
  case MAJOR_SETTING_OTA:
    return 56;
  case MAJOR_SETTING_SYS_CONFIG_DEFAULT:
    return 55;
  case MAJOR_SETTING_SLEEP:
    return 57;
  case 0xFFFF: // Dùng tạm Major đặc biệt để map bit 63 nếu cần
    return 63;
  default:
    return 63; // Unmapped
  }
}

void tx_ble_on_ack_received(const uint8_t *p_payload)
{
  if (!m_waiting_for_ack || m_p_items == NULL)
    return;

  // 1. Kiểm tra Encryption Key (Byte 2-7) - Phải khớp gói vừa phát
  if (memcmp(&p_payload[2], m_base_data.encryption_key, 6) != 0)
    return;

  // 2. Kiểm tra ID hoán đổi (ACK Base == Sent Tag, ACK Tag == Sent Base)
  bool id_match = (memcmp(&p_payload[8], m_base_data.id_tag, 5) == 0) &&
                  (memcmp(&p_payload[13], m_base_data.id_base, 5) == 0);
  if (!id_match)
    return;

  // 3. Kiểm tra nội dung (Major + Minor bao gồm cả CRC)
  bool content_match = (p_payload[18] == m_base_data.struct_id) &&
                       (p_payload[19] == m_base_data.struct_offset) &&
                       (p_payload[20] == m_base_data.crc_minor) &&
                       (p_payload[21] == m_base_data.value_to_change);

  if (content_match)
  {
    // printf("[BLE] >>> ACK MATCH from Tag ID: %02X:%02X:%02X:%02X:%02X\n",
    //        m_base_data.id_tag[0], m_base_data.id_tag[1],
    //        m_base_data.id_tag[2], m_base_data.id_tag[3],
    //        m_base_data.id_tag[4]);
    printf("[BLE] >>> ACK MATCH for Segment %d (Major: 0x%02X%02X)\n",
           m_current_idx, m_base_data.struct_id, m_base_data.struct_offset);

    uint16_t major =
        ((uint16_t)m_base_data.struct_id << 8) | m_base_data.struct_offset;
    uint8_t bit_idx = get_fragment_bit_index(major);
    if (bit_idx < 64)
    {
      fragment_status |= (1ULL << bit_idx);
    }
    // --- ACK tung manh: bao SPI ngay sau moi ACK nhan duoc ---
    spi_send_ack_status_update();
    printf("[BLE] Unicast ACK fragment %d/%d (bit %d), status=0x%08X%08X\n",
           m_current_idx + 1, m_item_count, bit_idx,
           (uint32_t)(fragment_status >> 32), (uint32_t)(fragment_status & 0xFFFFFFFF));

    // Kích hoạt callback khi một mảnh được ACK để xử lý logic tức thời (như
    // Mode 5)
    ducthang_ble_on_fragment_ack(major, m_base_data.value_to_change);

    rx_ble_scan_stop();
    m_waiting_for_ack = false;

    m_current_idx++;
    if (m_current_idx < m_item_count)
    {
      // QUAN TRỌNG: Phải bật lại Scan TRƯỚC khi phát mảnh tiếp
      // (vì rx_ble_scan_stop() ở trên đã tắt scan)
      rx_ble_scan_start();
      app_timer_stop(m_adv_update_timer);
      app_timer_start(m_adv_update_timer, APP_TIMER_TICKS(100), NULL);
      adv_update_timeout_handler(NULL);
    }
    else
    {
      printf("[BLE] Fragmentation COMPLETED (%d items)\n", m_item_count);
      fragment_status |= (1ULL << 63); // SET FINISH FLAG
      spi_send_ack_status_update();    // Báo ESP32: Đã xong hoàn toàn!
      app_timer_stop(m_adv_update_timer);
      m_p_items = NULL;

      // BÁO VỀ DUCTHANGBLE ĐỂ DỪNG TIMER GLOBAL VÀ CHUYỂN MODE
      ducthang_ble_on_config_done();
    }
  }
}

void tx_ble_on_tag_fragment_received(const uint8_t *p_payload)
{
  // 0. Chỉ xử lý Fragment từ Tag nếu Base đang ở chế độ Request (val_request >
  // 0) Hoặc nếu Base đang không bận đợi ACK từ Tag (tránh nhầm Fragment với
  // ACK)
  extern beacon_cfg_t g_beacon_cfg;
  if (g_beacon_cfg.val_request == 0)
    return;

  if (m_waiting_for_ack)
    return;

  extern uint8_t my_base_id_raw[5];
  extern uint8_t current_tag_id_raw[5];

  // 1. Kiểm tra Encryption Key (Byte 2-7)
  uint8_t expected_key[6] = {0x14, 0x05, 0x02, 0x20, 0x08, 0x03};
  if (memcmp(&p_payload[2], expected_key, 6) != 0)
    return;

  // 2. Kiểm tra ID: Khi Tag gửi Fragment cho Base, ID_BASE (byte 8-12) sẽ là
  // Tag ID, và ID_TAG (byte 13-17) sẽ là Base ID (Tag đã đảo ID_BASE với
  // ID_TAG)
  bool dest_match = (memcmp(&p_payload[13], my_base_id_raw, 5) == 0);
  if (!dest_match)
    return;

  bool src_match = (memcmp(&p_payload[8], current_tag_id_raw, 5) == 0);
  if (!src_match)
  {
    // Nếu chúng ta đang ở chế độ Request, có thể Tag đang phản hồi bằng Tag ID
    // thật của nó (thay vì MAC). Ta sẽ "uốn nắn" ID mục tiêu theo Tag này để
    // quá trình ACK diễn ra thông suốt.
    memcpy(current_tag_id_raw, &p_payload[8], 5);
  }

  // 3. Extract Major, Minor
  uint16_t major = ((uint16_t)p_payload[18] << 8) | p_payload[19];
  uint8_t minor = p_payload[21];

  printf(
      "[BLE] <<< Fragment Received from Tag (Major: 0x%04X, Value: 0x%02X)\n",
      major, minor);

  // 4. Send ACK back to Tag
  // Base ACK must be formatted exactly as Tag expects: ID_BASE = Base ID,
  // ID_TAG = Tag ID.
  tx_ble_custom_data_t ack_data;
  memset(&ack_data, 0, sizeof(ack_data));
  memcpy(ack_data.encryption_key, &p_payload[2], 6);
  memcpy(ack_data.id_base, my_base_id_raw, 5);
  memcpy(ack_data.id_tag, current_tag_id_raw, 5);
  ack_data.struct_id = p_payload[18];
  ack_data.struct_offset = p_payload[19];
  ack_data.crc_minor = p_payload[20];
  ack_data.value_to_change = p_payload[21];
  ack_data.measured_rssi = p_payload[22];

  tx_ble_stop();
  // Phát ACK 6 lần, mỗi lần cách nhau 50ms (50 / 0.625 = 80)
  // printf("[BLE] Sending ACK 6 times (Interval 50ms)...\n");
  tx_ble_update_data_and_start(&ack_data, 3, 80);

  // QUAN TRỌNG: Bật lại bộ quét ngay lập tức để không bỏ lỡ Fragment tiếp theo
  // của Tag
  rx_ble_scan_start();

  // 5. Cập nhật dữ liệu vào Struct Request Data
  request_on_tag_fragment_received(major, minor);
}
