#ifndef TX_BLE_H__
#define TX_BLE_H__

#include <stdbool.h>
#include <stdint.h>

// ==== DEFINITIONS FOR MAJOR VALUE (High Byte | Low Byte) ====

// High Byte 0x00: Struct TIMER: .SEND_TAG
#define MAJOR_TIMER_SEND_TAG_MOTION 0x0000
#define MAJOR_TIMER_SEND_TAG_STAND 0x0001
#define MAJOR_TIMER_SEND_TAG_SLEEP_MODE_1 0x0002
#define MAJOR_TIMER_SEND_TAG_SLEEP_MODE_2 0x0003
#define MAJOR_TIMER_SEND_TAG_SLEEP_MODE_3 0x0004

// High Byte 0x01: Struct TIMER: .SLEEP
#define MAJOR_TIMER_SLEEP_MODE_1 0x0100
#define MAJOR_TIMER_SLEEP_MODE_2 0x0101
#define MAJOR_TIMER_SLEEP_MODE_3 0x0102

// High Byte 0x02: Struct: .BATT
#define MAJOR_BATT_UPDATE_DEFAULT 0x0200
#define MAJOR_BATT_UPDATE_HIGH 0x0201
#define MAJOR_BATT_INCREASE 0x0202
#define MAJOR_BATT_DECREASE 0x0203

// High Byte 0x10: Struct CONFIG
#define MAJOR_CONFIG_UWB_CHAN 0x1000
#define MAJOR_CONFIG_UWB_PLEN 0x1001
#define MAJOR_CONFIG_UWB_PAC 0x1002
#define MAJOR_CONFIG_UWB_TXCODE 0x1003
#define MAJOR_CONFIG_UWB_RXCODE 0x1004
#define MAJOR_CONFIG_UWB_SFDTYPE 0x1005
#define MAJOR_CONFIG_UWB_DATARATE 0x1006
#define MAJOR_CONFIG_UWB_PHRMODE 0x1007
#define MAJOR_CONFIG_UWB_PHRRATE 0x1008
#define MAJOR_CONFIG_UWB_SFDTO 0x1009
#define MAJOR_CONFIG_UWB_STSMODE 0x100A
#define MAJOR_CONFIG_UWB_STSLEN 0x100B
#define MAJOR_CONFIG_UWB_PDOA 0x100C

// High Byte 0x20: Struct STATE
#define MAJOR_STATE_DEFAULT 0x2000
#define MAJOR_STATE_TX 0x2001
#define MAJOR_STATE_OFF_UWB 0x2002
#define MAJOR_STATE_SOS 0x2003
#define MAJOR_STATE_IDENTIFY 0x2004
#define MAJOR_STATE_TWR 0x2005
#define MAJOR_STATE_RESET 0x2006
#define MAJOR_STATE_AIRPLAN 0x2007
#define MAJOR_STATE_MOTION 0x2008
#define MAJOR_STATE_NO_CHANGE 0x20FE // Ký hiệu giữ nguyên State hiện tại

// High Byte 0x30: Struct REQUEST
#define MAJOR_REQUEST_TIMER 0x3000
#define MAJOR_REQUEST_CONFIG 0x3010
#define MAJOR_REQUEST_STATE 0x3020
#define MAJOR_REQUEST_ID_TAG 0x30EE

// High Byte 0xEE: Frame CHANGE TAG ID
#define MAJOR_ID_CHANGE_FLAG 0xEE00  // Cờ báo hiệu đổi ID
#define MAJOR_ID_CHANGE_BYTE1 0xEE01 // Byte 0 của ID mới

extern uint64_t fragment_status;
#define MAJOR_ID_CHANGE_BYTE2 0xEE02 // Byte 1 của ID mới
#define MAJOR_ID_CHANGE_BYTE3 0xEE03 // Byte 2 của ID mới
#define MAJOR_ID_CHANGE_BYTE4 0xEE04 // Byte 3 của ID mới
#define MAJOR_ID_CHANGE_BYTE5 0xEE05 // Byte 4 của ID mới

#define MAJOR_ID_LAST_BYTE1 0xEE06 // Byte 0
#define MAJOR_ID_LAST_BYTE2 0xEE07 // Byte 1
#define MAJOR_ID_LAST_BYTE3 0xEE08 // Byte 2
#define MAJOR_ID_LAST_BYTE4 0xEE09 // Byte 3
#define MAJOR_ID_LAST_BYTE5 0xEE0A // Byte 4

#define MAJOR_MAC_ADDRESS_BYTE1 0xEE0B // Byte 0
#define MAJOR_MAC_ADDRESS_BYTE2 0xEE0C // Byte 1
#define MAJOR_MAC_ADDRESS_BYTE3 0xEE0D // Byte 2
#define MAJOR_MAC_ADDRESS_BYTE4 0xEE0E // Byte 3
#define MAJOR_MAC_ADDRESS_BYTE5 0xEE0F // Byte 4

#define MAJOR_APPLY_CONFIG_TO_BASE 0xEE10 // Cờ áp dụng cấu hình Base sau khi phát
#define MAJOR_BROADCAST_FLAG 0xEE11       // Cờ cấu hình Broadcast mode

// High Byte 0x40: Struct SETTING
#define MAJOR_SETTING_CHARGE_TX 0x4000             // CHARGE_TX: minor=0x00(OFF), 0xFF(ON)
#define MAJOR_SETTING_OTA 0x4001                   // OTA: minor=0xFF(ENABLE)
#define MAJOR_SETTING_SYS_CONFIG_DEFAULT 0x4002    // SYS_CONFIG_DEFAULT: minor=0xFF(reset to default)
#define MAJOR_SETTING_SLEEP 0x4003                 // SLEEP: minor=0x00(DISABLE), 0xFF(ENABLE)

// ==== Định nghĩa cấu trúc dữ liệu theo yêu cầu ====
// Tổng cộng frame iBeacon (sau Company ID) là 23 bytes:
// Type (1) + Len (1) + UUID (16) + Major (2) + Minor (2) + RSSI (1)
// Các trường Key, Count, Index, IDs nằm trong vùng UUID (16 bytes)

typedef struct {
  uint8_t encryption_key[6]; // KEY MÃ HÓA (6 bytes)
  uint8_t id_base[5];        // ID BASE (5 bytes)
  uint8_t id_tag[5];         // ID TAG (5 bytes)
  uint8_t struct_id;         // ĐỊNH DANH STRUCT (1 byte) -> Major High
  uint8_t struct_offset;     // ĐỊNH DANH VỊ TRÍ STRUCT (1 byte) -> Major Low
  uint8_t crc_minor;         // CRC_MINOR_LOW_BYTE (1 byte) -> Minor High
  uint8_t value_to_change;   // GIÁ TRỊ MUỐN THAY ĐỔI (1 byte) -> Minor Low
  int8_t measured_rssi;      // RSSI tại 1m (Measured RSSI)
} tx_ble_custom_data_t;

// Struct cặp giá trị cấu hình (Major quy định loại, Minor là giá trị)
typedef struct {
  uint16_t major;
  uint8_t minor;
} tx_ble_cfg_pair_t;

/**
 * @brief Bắt đầu phát quảng bá phân mảnh (Gửi danh sách cấu hình lần lượt)
 *
 * @param[in] p_base_data Dữ liệu nền (Key, ID, Struct ID...) dùng chung cho các
 * gói. (Lưu ý: packet_count, packet_index, major, minor trong này sẽ bị ghi đè
 * tự động)
 * @param[in] p_items     Mảng các cặp cấu hình {Major, Minor} cần gửi
 * @param[in] item_count  Số lượng cặp cấu hình
 * @param[in] interval_ms Thời gian chuyển đổi giữa các mảnh (ví dụ 200ms)
 */
void tx_ble_start_fragmented_advertising(tx_ble_custom_data_t *p_base_data,
                                         tx_ble_cfg_pair_t *p_items,
                                         uint8_t item_count,
                                         uint32_t interval_ms);

/**
 * @brief Hàm bắt đầu phát quảng bá Broadcast (không cần ACK)
 */
void tx_ble_start_broadcast_advertising(tx_ble_custom_data_t *p_base_data,
                                         tx_ble_cfg_pair_t *p_items,
                                         uint8_t item_count,
                                         uint32_t interval_ms);

/**
 * @brief Khởi tạo module TX BLE Beacon
 */
void tx_ble_init(void);

/**
 * @brief Cập nhật dữ liệu và bắt đầu phát quảng bá (Advertising)
 *
 * @param[in] p_data Pointer đến cấu trúc dữ liệu tùy chỉnh
 */
void tx_ble_update_data_and_start(tx_ble_custom_data_t *p_data, uint8_t max_evts,
                                 uint16_t interval_0_625ms);

/**
 * @brief Dừng phát quảng bá
 */
void tx_ble_stop(void);

/**
 * @brief Xử lý khi nhận được gói ACK từ Node B
 * @param[in] p_payload Trỏ tới buffer 21 bytes payload iBeacon (bỏ 2 byte
 * header Type/Len)
 */
void tx_ble_on_ack_received(const uint8_t *p_payload);

/**
 * @brief Xử lý khi nhận được Response Fragment từ Tag (khi ở Request Mode)
 * @param[in] p_payload Trỏ tới buffer 21 bytes payload iBeacon
 */
void tx_ble_on_tag_fragment_received(const uint8_t *p_payload);

#endif // TX_BLE_H__
