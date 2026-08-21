#ifndef REQUEST_H__
#define REQUEST_H__

#include <stdbool.h>
#include <stdint.h>

// ============================================================
//  1. STRUCT TIMER – Lưu giá trị timer hiện tại của Tag
// ============================================================
typedef struct {
  uint8_t send_tag_motion; // Timer gửi tag khi motion
  uint8_t send_tag_stand;  // Timer gửi tag khi đứng yên
  uint8_t send_tag_sleep1; // Timer gửi tag sleep mode 1
  uint8_t send_tag_sleep2; // Timer gửi tag sleep mode 2
  uint8_t send_tag_sleep3; // Timer gửi tag sleep mode 3

  uint8_t sleep_mode1; // Thời gian sleep mode 1
  uint8_t sleep_mode2; // Thời gian sleep mode 2
  uint8_t sleep_mode3; // Thời gian sleep mode 3

  uint8_t batt_default;  // Battery update default
  uint8_t batt_high;     // Battery update high
  uint8_t batt_increase; // Battery increase threshold
  uint8_t batt_decrease; // Battery decrease threshold
} __attribute__((packed)) request_timer_t;

// ============================================================
//  2. STRUCT CONFIG – Lưu cấu hình UWB hiện tại của Tag
// ============================================================
typedef struct {
  uint8_t uwb_chan;     // Channel (5 or 9)
  uint8_t uwb_plen;     // Preamble length
  uint8_t uwb_pac;      // PAC size
  uint8_t uwb_txcode;   // TX preamble code
  uint8_t uwb_rxcode;   // RX preamble code
  uint8_t uwb_sfdtype;  // SFD type
  uint8_t uwb_datarate; // Data rate
  uint8_t uwb_phrmode;  // PHR mode
  uint8_t uwb_phrrate;  // PHR rate
  uint16_t uwb_sfdto;   // SFD timeout
  uint8_t uwb_stsmode;  // STS mode
  uint8_t uwb_stslen;   // STS length
  uint8_t uwb_pdoa;     // PDoA mode
} __attribute__((packed)) request_config_t;

// ============================================================
//  3. TRẠNG THÁI HIỆN TẠI – State hiện tại của Tag
// ============================================================
typedef enum {
  TAG_STATE_DEFAULT = 0,
  TAG_STATE_TX = 1,
  TAG_STATE_OFF_UWB = 2,
  TAG_STATE_SOS = 3,
  TAG_STATE_IDENTIFY = 4,
  TAG_STATE_TWR = 5,
  TAG_STATE_RESET = 6,
  TAG_STATE_AIRPLAN = 7,
  TAG_STATE_MOTION = 8,
} tag_state_e;

// ============================================================
//  4. STRUCT ID TAG – Lưu thông tin ID của Tag
// ============================================================
typedef struct {
  uint8_t id[5]; // ID Tag (5 bytes), VD: 10 26 25 21 03
} __attribute__((packed)) request_id_tag_t;

// ============================================================
//  5. STRUCT TỔNG HỢP – Chứa toàn bộ dữ liệu request
// ============================================================
typedef struct {
  request_timer_t timer;
  request_config_t config;
  uint32_t current_state;
  request_id_tag_t id_tag;
} __attribute__((packed)) request_data_t;

// ============================================================
//  BIẾN TOÀN CỤC
// ============================================================
extern request_data_t g_request_data;

// ============================================================
//  HÀM XỬ LÝ REQUEST
// ============================================================
void request_init(void);

/**
 * @brief Xử lý khi nhận được gói request từ Tag
 *        Đọc request type rồi lưu lại dữ liệu tương ứng
 *
 * @param[in] major  Mã loại cấu hình
 * @param[in] minor  Giá trị cấu hình
 */
void request_on_tag_fragment_received(uint16_t major, uint8_t minor);
void request_print_data(uint8_t request_type);

#endif // REQUEST_H__
