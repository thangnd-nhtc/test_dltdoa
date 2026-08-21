#ifndef HANDEL_ISP3080_H
#define HANDEL_ISP3080_H

#include <stddef.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>

#define jitter_pin 21
#define sync_enable_pin 26
#define rst_isp3080 27
#define PIN_IRQ_FROM_NRF 35 // GPIO35 (input-only)

#include "handle_com_regs.h"
#include "handle_decawave.h"

/* === Chân SPI tương ứng với HSPI bạn đã chọn === */
#define ISP3080_SPI_PIN_SCK 14
#define ISP3080_SPI_PIN_MISO 12 // 13 HACHI APHI
#define ISP3080_SPI_PIN_MOSI 13 // 12 HACHI APHI
#define ISP3080_SPI_PIN_CS 16   //

const uint32_t my_device_id = 125201;

#define CMD_BEGIN_DW3000 0x01
#define CMD_ENABLE_RX 0x02
#define CMD_GET_FRAME_SERVER 0x03
#define CMD_SET_DEVICE_ID 0x04
#define CMD_DO_DS_TWR 0x05
#define CMD_GET_FRAME_DW 0x06
#define CMD_SEND_OFFSET 0x07
#define CMD_SET_BEACON_DYNAMIC                                                 \
  0x10 // Cấu hình Beacon linh hoạt (ID/Value pairs)
#define CMD_GET_SS_TWR_DISTANCE 0x11 // Lấy kết quả SS-TWR DucThang
#define CMD_GET_FRAGMENT_STATUS 0x12 // Lấy trạng thái ACK các mảnh cấu hình

// ---- MAJOR IDs for Beacon Config (Sync with nRF) ----
// ---- OTA Commands for ISP3080 ----
#define CMD_OTA_ENTER   0x20
#define CMD_OTA_DATA    0x21
#define CMD_OTA_END     0x22
#define CMD_OTA_ABORT   0x23
#define CMD_OTA_STATUS  0x24

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

// High Byte 0x04: Struct: .UART
#define MAJOR_UART_LED 0x0400

// High Byte 0x05: Struct: .CHARGE
#define MAJOR_CHARGE_NOISE 0x0500
#define MAJOR_CHARGE_UPDATE 0x0501

// High Byte 0x06: Struct: .LED1
#define MAJOR_LED1_BLINK 0x0600
#define MAJOR_LED1_BLINK_MS 0x0601

// High Byte 0x07: Struct: .AIRPLAN
#define MAJOR_AIRPLAN_MODE 0x0700

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

// High Byte 0x30: Struct REQUEST
#define MAJOR_REQUEST_TIMER 0x3000
#define MAJOR_REQUEST_CONFIG 0x3010
#define MAJOR_REQUEST_STATE 0x3020
#define MAJOR_REQUEST_ID_TAG 0x30EE

#define CMD_GET_REQUEST_DATA 0x13 // Lệnh lấy dữ liệu Request mới thêm
#define CMD_GET_BCAST_TWR_RESULT 0x14 // Poll queue BSS-TWR
#define CMD_GET_ISP_VERSION     0x15 // Lấy version FW/HW của ISP3080

// High Byte 0xEE: Frame CHANGE TAG ID
#define MAJOR_ID_CHANGE_FLAG 0xEE00  // Cờ báo hiệu đổi ID
#define MAJOR_ID_CHANGE_BYTE1 0xEE01 // Byte 0 của ID mới
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
#define MAJOR_ENABLE_BCAST_TWR 0xEE12      // Cờ Enable Broadcast TWR

// High Byte 0x40: Struct SETTING
#define MAJOR_SETTING_CHARGE_TX 0x4000             // CHARGE_TX: minor=0x00(OFF), 0xFF(ON)
#define MAJOR_SETTING_OTA 0x4001                   // OTA: minor=0xFF(ENABLE)
#define MAJOR_SETTING_SYS_CONFIG_DEFAULT 0x4002    // SYS_CONFIG_DEFAULT: minor=0xFF(reset to default)
#define MAJOR_SETTING_SLEEP 0x4003                 // SLEEP: minor=0x00(DISABLE), 0xFF(ENABLE)

#define SPI_FRAME_TYPE_SERVER 0x00
#define SPI_FRAME_TYPE_DW 0x01
#define SPI_FRAME_TYPE_OFFSET 0x02
#define SPI_FRAME_TYPE_REQUEST 0x04 // Đã đổi thành 0x04 cho khác biệt hoàn toàn
#define SPI_FRAME_TYPE_ACK_STATUS 0x05 // New: Nhận trạng thái ACK mảng
#define SPI_FRAME_TYPE_BCAST_TWR_RESULT 0x06 // Broadcast TWR Result

#define STATUS_OK 0x00
#define FRAME_TYPE_OFFSET 0x02

#define BUF_LEN 512 // Độ dài buffer tối đa cho giao tiếp SPI

#define DECAWAVE_MASTER_ACCESS_NUM 4

#define MAX_FRAME_HISTORY 2

typedef struct {
  uint32_t packet_id;
  uint32_t tag_id;
} frame_entry_t;

extern server_dataframe_t server_frame_rx;
extern dw_dataframe_t frame_twr;
extern dw_dataframe_t frame_ofsset_rx;
extern bool final_twr;

#define REQUEST_UPLOAD_CMD 97 // CMD ID cho việc đẩy dữ liệu Request lên Server

extern request_data_t g_request_data_rx; // Biến lưu dữ liệu request nhận được
extern uint64_t g_fragment_status_rx;    // Biến lưu trạng thái ACK mảng nhận được

typedef struct {
  uint8_t Type_data;
  uint8_t Packit_ID[4];
  uint8_t Serial_ID[4];
  uint8_t Timestamp[5];
  struct {
    uint8_t Serial[4];
    uint8_t Timestamp[8];
  } Mts_access[3];
  uint8_t Tag_ID[4];
  uint8_t Motion;
  uint8_t Button;
  uint8_t Free_fall;
  uint8_t RSSI;
} server_dataframe_send;

// khai báo hàm check ack isp3080
extern uint32_t last_quiet_ms; // thời điểm cuối cùng có phản hồi nRF
#define QUIET_MS 310           // thời gian im lặng tối đa
#define MAX_TRY 2              // số lần thử đọc lại nRF
#define RETRY_GAP 10           // delay giữa các lần thử (ms)
extern bool flag_isp3080;      // thời điểm cuối cùng có phản hồi nRF
//

void reset_module_jitter(void);
void reset_isp3080();
bool begin_dw3000();                           // Khởi tạo DW3000
bool enable_rx_mode();                         // Bật chế độ nhận dữ liệu
void print_request_data(const request_data_t *data);
uint8_t read_frame_from_nrf();                 // Đọc frame từ nRF và xử lý
bool set_device_id_to_nrf(uint32_t device_id); // Gửi ID thiết bị đến nRF
extern uint8_t temp_id_mode;                   // PTB: Bien tam luu val_id_mode
extern bool g_bcast_twr_active;                // Co tam luu trang thai BSS-TWR
int build_csv_from_server_frame(
    const server_dataframe_t *frame, char *buffer,
    size_t bufsize); // Xây dựng chuỗi CSV từ frame server_dataframe_t
bool send_ds_twr_command(uint32_t target_id); // Hàm đo khoảng cách
uint64_t parse_ts_40bit(const uint8_t ts[5]);

void isp3080_spi_init(uint32_t freq_hz);
void isp3080_spi_cs_assert(void);
void isp3080_spi_cs_release(void);
uint8_t isp3080_spi_txrx8(uint8_t tx);
void isp3080_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len);
void isp3080_spi_write(const uint8_t *buf, size_t len);
void isp3080_spi_read(uint8_t *buf, size_t len);

bool send_cmd_to_isp3080(uint8_t cmd, const uint8_t *payload,
                         size_t payload_len, uint8_t *response,
                         size_t response_len, uint32_t timeout_ms = 500);
bool get_dw_frame_from_nrf(dw_dataframe_t *out_frame);
void print_dw_dataframe(const dw_dataframe_t *frame);
bool get_server_frame_from_nrf(server_dataframe_t *out_frame);
void print_server_dataframe(const server_dataframe_t *f);
bool check_ack_isp3080(void); // PTB check ack isp3080
bool set_beacon_config_to_nrf(beacon_cfg_t *cfg);

// --- CÁC HÀM GIAO TIẾP OTA VỚI ISP3080 ---
bool isp3080_ota_enter(uint32_t fw_size, uint32_t crc32);
bool isp3080_ota_data(uint32_t offset, const uint8_t *data, uint32_t len);
bool isp3080_ota_end(void);
bool isp3080_ota_abort(void);

// --- Đọc ISP3080 version qua SPI (retry 3 lần) ---
bool read_isp_version(char *fw_ver, size_t fw_len, char *hw_ver, size_t hw_len);

#endif /* HANDEL_ISP3080_H */
