#ifndef MAIN_H
#define MAIN_H

/*─────────────────────────────────────────────*
 *                  INCLUDE                     *
 *─────────────────────────────────────────────*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "DW3000.h" // Giao tiếp DW3000 API
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrfx_spis.h"
#include "tx_ble.h"

/*─────────────────────────────────────────────*
 *            PIN MAPPING – SPIS1               *
 *─────────────────────────────────────────────*/
#define SPIS_SCK_PIN NRF_GPIO_PIN_MAP(0, 14)
#define SPIS_MOSI_PIN NRF_GPIO_PIN_MAP(0, 26) // 12
#define SPIS_MISO_PIN NRF_GPIO_PIN_MAP(0, 12) // 26
#define SPIS_CSN_PIN NRF_GPIO_PIN_MAP(0, 11)

/*─────────────────────────────────────────────*
 *           ISP3080 VERSION                     *
 *─────────────────────────────────────────────*/
#define ISP_FW_VERSION "1.02"
#define ISP_HW_VERSION "2.00"

/*─────────────────────────────────────────────*
 *           COMMAND MÃ CHỨC NĂNG SPI           *
 *─────────────────────────────────────────────*/
#define CMD_BEGIN_DW3000 0x01     // Bắt đầu khởi tạo DW3000
#define CMD_ENABLE_RX 0x02        // Bật chế độ nhận gói tin từ DW3000
#define CMD_GET_FRAME_SERVER 0x03 // Lấy gói dữ liệu từ DW3000
#define CMD_SET_DEVICE_ID 0x04    // Thiết lập ID thiết bị (MY_DEVICE_ID)
#define CMD_DO_DS_TWR \
  0x05                            // ESP32 yêu cầu đo khoảng cách DS-TWR (TWR active mode)
#define CMD_GET_FRAME_DW 0x06     // Lấy gói DW từ DW3000
#define CMD_SEND_TX_OFFSET 0x07   // Gửi gói tin Offset đến DW3000
#define CMD_GET_OFFSET_FRAME 0x08 // Lấy gói DW Offset từ DW3000
#define CMD_SET_BEACON_STR 0x0F   // Thiết lập cấu hình Beacon
#define CMD_SET_BEACON_DYNAMIC \
  0x10                               // Cấu hình Beacon linh hoạt (ID/Value pairs)
#define CMD_GET_SS_TWR_DISTANCE 0x11 // Lấy kết quả SS-TWR DucThang
#define CMD_GET_FRAGMENT_STATUS \
  0x12                                // Kiểm tra trạng thái ACK các mảnh cấu hình
#define CMD_GET_REQUEST_DATA 0x13     // Lấy dữ liệu Request từ Tag (giá trị hiện tại)
#define CMD_GET_BCAST_TWR_RESULT 0x14 // Poll kết quả BSS-TWR từ queue (thay IRQ)
#define CMD_GET_ISP_VERSION 0x15      // Lấy version FW/HW của ISP3080
#define MAJOR_ENABLE_BCAST_TWR 0xEE12 // Cờ Enable Broadcast TWR

#define FRAME_TYPE_SERVER 0x00
#define FRAME_TYPE_DW 0x01
#define FRAME_TYPE_OFFSET 0x02
#define FRAME_TYPE_REQUEST 0x04
#define FRAME_TYPE_ACK_STATUS 0x05       // New: Gói báo trạng thái ACK mảnh
#define FRAME_TYPE_BCAST_TWR_RESULT 0x06 // Broadcast TWR Result

#define OK_STATUS 0x00 // Trạng thái OK

// ---- Struct Kết quả Broadcast TWR (Dùng để gửi lên U7) ----
typedef struct
{
  uint8_t tag_id[5];
  uint32_t distance_mm;
} __attribute__((packed)) dw_twr_result_t;

// ==== Cấu trúc cấu hình Beacon (Đồng bộ với U6/U7) ====
typedef struct
{
  // --- Timer/Send Tag (5 trường) ---
  uint8_t val_motion;
  uint8_t val_stand;
  uint8_t val_sleep1;
  uint8_t val_sleep2;
  uint8_t val_sleep3;
  // --- Sleep Mode (3 trường) ---
  uint8_t val_mode1;
  uint8_t val_mode2;
  uint8_t val_mode3;

  // --- Battery (4 trường) ---
  uint8_t val_batt_default;
  uint8_t val_batt_high;
  uint8_t val_batt_inc;
  uint8_t val_batt_dec;

  // --- ID Configuration ---
  uint8_t val_id_last[5]; // ID hiện tại (Target ID)
  uint8_t SerialID[5];    // Serial của thiết bị
  uint8_t val_id_new[5];  // ID mới muốn đặt
  uint8_t val_id_mode;
  uint8_t val_id_change; // Cờ báo hiệu có thay đổi ID (Checkbox Change ID)
  uint8_t val_request;   // Request frame (0x30)
  // --- UWB/DWT Configuration (13 trường) ---
  uint8_t uwb_chan;
  uint8_t uwb_plen;
  uint8_t uwb_pac;
  uint8_t uwb_txcode;
  uint8_t uwb_rxcode;
  uint8_t uwb_sfdtype;
  uint8_t uwb_datarate;
  uint8_t uwb_phrmode;
  uint8_t uwb_phrrate;
  uint16_t uwb_sfdto;
  uint8_t uwb_stsmode;
  uint8_t uwb_stslen;
  uint8_t uwb_pdoa;
  // --- Flag áp dụng cấu hình ---
  uint8_t apply_config_to_base; // 1: Ap dung cho Base sau khi phat xong, 0: Chi phat qua BLE

  // --- MAC Address (5 bytes = 10 hex chars) ---
  uint8_t mac_address[5]; // MAC address của thiết bị (VD: "1023ABCDEF")

  // --- Broadcast Mode ---
  uint8_t broadcast;        // 1: Broadcast cho tất cả Tag (không ACK, 30s/fragment), 0: Unicast
  uint8_t enable_bcast_twr; // 1: Enable auto-scanning and sequential TWR measuring

  // --- Setting (0x40) ---
  uint8_t charge_tx;          // 1: OFF, 2: ON
  uint8_t ota_enable;         // 1: ENABLE OTA
  uint8_t sys_config_default; // 1: reset tag SYS_CONFIG về mặc định
  uint8_t sleep_enable;       // 1: DISABLE SLEEP, 2: ENABLE SLEEP
} __attribute__((packed)) beacon_cfg_t;

/*─────────────────────────────────────────────*
 *           BUFFER KÍCH THƯỚC SPI              *
 *─────────────────────────────────────────────*/
#define BUF_LEN 512
#define RX_BUF_SIZE 512

/*─────────────────────────────────────────────*
 *       TDOA SERVER FIFO – CHỐNG GHI ĐÈ       *
 *─────────────────────────────────────────────*/
#ifndef TDOA_SERVER_QUEUE_SIZE
#define TDOA_SERVER_QUEUE_SIZE 64
#endif

extern volatile uint32_t tdoa_uwb_received;
extern volatile uint32_t tdoa_queue_pushed;
extern volatile uint32_t tdoa_queue_dropped;
extern volatile uint32_t tdoa_spi_popped;

/*─────────────────────────────────────────────*
 *           FLAG – TRẠNG THÁI GIAO TIẾP        *
 *─────────────────────────────────────────────*/
extern bool enable_polling_rx;
// extern volatile bool has_tag_frame_ready;
extern volatile uint64_t last_rx_timestamp;

/*─────────────────────────────────────────────*
 *              BUFFER SPI                      *
 *─────────────────────────────────────────────*/
extern uint8_t m_tx_buf[BUF_LEN];      // Buffer gửi
extern uint8_t m_rx_buf[BUF_LEN];      // Buffer nhận
extern uint8_t m_rx_copy[RX_BUF_SIZE]; // Buffer bản sao dùng cho log/debug

/*─────────────────────────────────────────────*
 *         SPIS1 – THỰC THỂ & BIẾN CỜ           *
 *─────────────────────────────────────────────*/
static const nrfx_spis_t spis = NRFX_SPIS_INSTANCE(1);
static volatile bool spis_xfer_done = false;
static volatile size_t spis_rx_len = 0;

/*─────────────────────────────────────────────*
 *             HÀM NỘI BỘ STATIC                *
 *─────────────────────────────────────────────*/
static void reset_pin_init(void);                       // Khởi tạo chân reset mềm
static void irq_esp_init(void);                         // Khởi tạo chân IRQ kéo LOW
static void dump_bytes(const uint8_t *buf, size_t len); // In dữ liệu dạng HEX
static void spis_evt_handler(nrfx_spis_evt_t const *evt,
                             void *ctx); // Callback SPIS

/*─────────────────────────────────────────────*
 *            HÀM DÙNG NGOÀI (GLOBAL)           *
 *─────────────────────────────────────────────*/
void check_reset_pin(void);  // Kiểm tra nút reset mềm (ESP32 điều khiển)
static void spis_init(void); // Khởi tạo SPIS1 (Slave SPI)

// ==== Flash persistence ====
extern bool flag_save_config_pending;
extern beacon_cfg_t g_pending_beacon_cfg;
void load_dwt_config(void);
void cache_pending_dwt_config(void);
void apply_and_save_dwt_config(void);

void spi_send_request_done(void);
void spi_send_ack_status_update(void); // New

// ==== BSS-TWR Result Queue ====
#define TWR_RESULT_QUEUE_SIZE 32
typedef struct
{
  uint8_t tag_id[5];
  uint32_t distance_mm;
} twr_queue_entry_t;
extern twr_queue_entry_t twr_result_queue[TWR_RESULT_QUEUE_SIZE];
extern uint8_t twr_q_head;
extern uint8_t twr_q_tail;
extern uint8_t twr_q_count;
void twr_queue_push(uint8_t *tag_id, uint32_t dist_mm);
bool twr_queue_pop(twr_queue_entry_t *out);
#endif
/* MAIN_H */
