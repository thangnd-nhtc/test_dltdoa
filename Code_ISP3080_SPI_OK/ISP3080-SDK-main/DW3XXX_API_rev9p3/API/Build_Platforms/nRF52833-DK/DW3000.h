#ifndef DW3000_H
#define DW3000_H

/*─────────────────────────────────────────────*
 *                  INCLUDE                     *
 *─────────────────────────────────────────────*/
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "main.h"
#include <boards.h>
#include <config_options.h>
#include <deca_device_api.h>
#include <deca_probe_interface.h>
#include <deca_spi.h>
#include <port.h>
#include <sdk_config.h>
#include <shared_defines.h>
#include <shared_functions.h>

/*─────────────────────────────────────────────*
 *               ĐỊNH NGHĨA CHUNG               *
 *─────────────────────────────────────────────*/
#define SIZE_OF_DATAFRAME 32
#define RX_BUF_MAX_LEN 127
#define RX_BUF_LEN 34
#define MAX_TAG_TRACKED 30
#define DECAWAVE_MASTER_ACCESS_NUM 4

#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

// #define POLL_TX_TO_RESP_RX_DLY_UUS (600 + CPU_PROCESSING_TIME)
#define POLL_TX_TO_RESP_RX_DLY_UUS 240 // 2000
#define RESP_RX_TIMEOUT_UUS 292400     // 20000 292400
#define RESP_RX_TIMEOUT_UUS_OFFSET 10000

#define PIN_TAG_IRQ NRF_GPIO_PIN_MAP(1, 0) // P1.00 → ESP32 GPIO35
#define TAG_IRQ_SET() nrf_gpio_pin_set(PIN_TAG_IRQ)
#define TAG_IRQ_CLR() nrf_gpio_pin_clear(PIN_TAG_IRQ)

static uint32_t packet_id_counter = 1; // Biến đếm gói tin

// khai báo trùng gói tin
#define MAX_TAG_TRACKED 30

typedef struct {
  uint32_t Serial_ID;
  uint32_t Packet_ID;
} tag_cache_entry_t;

extern tag_cache_entry_t tag_cache[MAX_TAG_TRACKED];
extern uint8_t tag_cache_index;

/*─────────────────────────────────────────────*
 *           ENUM VÀ CODE COMMAND               *
 *─────────────────────────────────────────────*/
typedef enum {
  Cmd_start_twr = 0,
  Cmd_Pool = 1,
  Cmd_Resp = 2,
  Cmd_Final = 3,
  Cmd_Distance = 4,
  Cmd_Sync = 5,
  Cmd_Tag = 7,
  Cmd_Offset = 8
} dw_command;

typedef enum {
  Cmd_tag_nomal = 0,
  Cmd_tag_sensor = 1,
  Cmd_tag_solut = 2,
  Cmd_tag_hoya = 3,
  Cmd_tag_dps422 = 4,
  cmd_custom_ducthang = 5
} dw_tag_command;

typedef enum {
  mode_wait,
  mode_tdoa,
  mode_twr,
  mode_sync_tx,
  mode_sync_rx,
  mode_offset_tx,
  mode_offset_rx
} dw_mode_run;

enum { DW_ERR_PROBE = 1, DW_ERR_INIT = 2, DW_ERR_CONFIG = 3 };

/*─────────────────────────────────────────────*
 *             CẤU HÌNH MẶC ĐỊNH DW3000         *
 *─────────────────────────────────────────────*/
  //static dwt_config_t config = {
  //    .chan = 9,
  //    .txPreambLength = DWT_PLEN_512,
  //    .rxPAC = DWT_PAC16,
  //    .txCode = 9,
  //    .rxCode = 9,
  //    .sfdType = 2,
  //    .dataRate = DWT_BR_850K,
  //    .phrMode = DWT_PHRMODE_STD,
  //    .phrRate = DWT_PHRRATE_STD,
  //    .sfdTO = (512 + 1 + 16 - 16),
  //    .stsMode = DWT_STS_MODE_OFF,
  //    .stsLength = DWT_STS_LEN_64,
  //    .pdoaMode = DWT_PDOA_M0};

// Thang 2048
// static const dwt_config_t config= {
//  .chan            = 9,
//  .txPreambLength  = DWT_PLEN_2048,
//  .rxPAC           = DWT_PAC16,
//  .txCode          = 9, .rxCode = 9,
//  .sfdType         = 3,
//  .dataRate        = DWT_BR_850K,
//  .phrMode         = DWT_PHRMODE_STD,
//  .phrRate         = DWT_PHRRATE_STD,
//  .sfdTO           = (2048 + 1 - 16 + 8),
//  .stsMode         = DWT_STS_MODE_SDC,
//  .stsLength       = DWT_STS_LEN_256,
//  .pdoaMode        = DWT_PDOA_M0
//};

// Thang 4096
// static const dwt_config_t config = {
//  .chan            = 9,                  // 5 ho?c 9 d?u h? tr?; ch?n 9 n?u
//  ph?n c?ng/anten hi?n t?i t?i uu cho ch9 .txPreambLength  = DWT_PLEN_4096, //
//  preamble d�i d? tang nh?y & b�m d?ng b? ch?c .rxPAC           = DWT_PAC16,
//  // 850 kb/s & preamble =128 ? PAC=16 l� khuy?n ngh? .txCode          = 9,
//  .rxCode = 9,    // code 9 ? PRF 64 MHz (b?n da du?ng) .sfdType         = 3,
//  // IEEE 802.15.4z 8-symbol SFD (c?ng nhi?u) .dataRate        = DWT_BR_850K,
//  // robust nh?t; c� th? n�ng l�n 6M8 khi c?n latency .phrMode         =
//  DWT_PHRMODE_STD, .phrRate         = DWT_PHRRATE_STD,   // d? PHR ? 850k nhu
//  chu?n m?c d?nh (PHR lu�n demod theo 850k) .sfdTO           = (4096 + 1 - 16
//  + 8), // =4089 theo c�ng th?c manual .stsMode         = DWT_STS_MODE_SDC, //
//  SDC = m� STS t?i uu ToA, b? qu?n l� key .stsLength       = DWT_STS_LEN_256,
//  // c� th? tang 512 n?u m�i tru?ng c?c nhi?u/d?ng g�i .pdoaMode        =
//  DWT_PDOA_M0
//};

extern dwt_config_t config;

// cau hinh test ok 02/08/2025 co vat can 13.5m, khong vat can 40m: On dinh
 //static dwt_config_t config = {
 // .chan = 9,
 // .txPreambLength = DWT_PLEN_1024,
 // .rxPAC = DWT_PAC16,
 // .txCode = 9,
 // .rxCode = 9,
 // .sfdType = 2,
 // .dataRate = DWT_BR_850K,
 // .phrMode = DWT_PHRMODE_STD,
 // .phrRate = DWT_PHRRATE_STD,
 // .sfdTO = (1024 + 1 + 16 - 16),
 // .stsMode = DWT_STS_MODE_OFF,
 // .stsLength = DWT_STS_LEN_64,
 // .pdoaMode = DWT_PDOA_M0};

// Chat GPT 07/08/2025 - Khả năng xuyên 1 người 12m
// static dwt_config_t config = {
//     .chan = 9,                              // Channel 9
//     .txPreambLength = DWT_PLEN_1024,       // Preamble dài → tăng độ ổn định
//     .rxPAC = DWT_PAC32,                    // PAC 32 → kháng nhiễu tốt
//     .txCode = 18,                          // Mã preamble cho channel 9 (theo
//     datasheet) .rxCode = 18, .sfdType = DWT_SFD_DW_16,           // Dùng SFD
//     chuẩn .dataRate = DWT_BR_850K,               // Tốc độ thấp, truyền xa
//     tốt .phrMode = DWT_PHRMODE_STD,            // PHR chuẩn (payload < 127
//     byte) .sfdTO = (1025 + 8 - 32),              // SFD Timeout cho preamble
//     1024, PAC 32 .stsMode = DWT_STS_MODE_OFF,             // Không dùng STS
//     .stsLength = 0,
//     .pdoaMode = DWT_PDOA_M0                // Không đo góc
// };

// C?u h�nh t?t nh?t d?n ng�y 08/08/2025
// -------------------------------------------------
// static dwt_config_t config = {
//    .chan = 9,                             // Channel 9
//    .txPreambLength = DWT_PLEN_4096,       // Preamble dài → tăng độ ổn định
//    .rxPAC = DWT_PAC32,                    // PAC 32 → kháng nhiễu tốt
//    .txCode = 18,                          // Mã preamble cho channel 9 (theo
//    datasheet) .rxCode = 18, .sfdType = DWT_SFD_DW_16,              // Dùng
//    SFD chuẩn .dataRate = DWT_BR_850K,               // Tốc độ thấp, truyền xa
//    tốt .phrMode = DWT_PHRMODE_STD,            // PHR chuẩn (payload < 127
//    byte) .phrRate = DWT_PHRRATE_STD, .sfdTO = (4097 + 16 - 32), // SFD
//    Timeout cho preamble 1024, PAC 32 .stsMode = DWT_STS_MODE_OFF, // Không
//    dùng STS .stsLength = DWT_STS_LEN_64, .pdoaMode = DWT_PDOA_M0 // Không đo
//    góc
//};

/*─────────────────────────────────────────────*
 *           STRUCT GÓI DỮ LIỆU TAG             *
 *─────────────────────────────────────────────*/
// typedef struct __attribute__((packed))
// {
//     uint32_t Des;
//     uint32_t Src;
//     uint32_t Packet_Id;
//     union
//     {
//         uint32_t U32_Init[4];
//         struct
//         {
//             uint8_t reserved[16];
//         } NORM;
//         struct
//         {
//             uint32_t Compass, Pressure, Acceleration;
//         } ETAG;
//         struct
//         {
//             uint8_t Temper, Humi, Vibra, reserved[13];
//         } SOLUT;
//         struct
//         {
//             uint16_t Temper;
//             uint32_t Pressure;
//             uint8_t reserved[10];
//         } DPS422;
//     } PAYLOAD;
//     uint8_t Cmd; // tag_data_type_t
//     uint8_t TypeDev;
//     uint16_t DCRC;
// } tag_frame_t;
typedef struct {
  uint32_t Des;                // Dia chi Dich
  uint32_t Src;                // Dia chi Nguon
  volatile uint32_t Packet_Id; // Packet_Id
  union {
    uint32_t U32_Init[4];
    /* Goi tin SYNC -------------------------*/
    struct {
      uint8_t Ts[5]; // Timestamp
    } SYNC;          // 4bytes
    /* Goi tin DISTANCE ---------------------*/
    struct {
      uint32_t Dis;
    } DIST; // 4bytes
    /* Goi tin Two-way ----------------------*/
    struct {
      struct {
        uint8_t poll_tx[5];  // poll_tx_ts
        uint8_t resp_rx[5];  // resp_rx_ts
        uint8_t final_tx[5]; // final_tx_ts
      } TIME_STAMP;
      uint8_t Cmd; //'N'(Normal), 'D'(Distance)
    } DS_TWR;      // 5*3 + 1 = 16
    /* Goi tin TAG ----------------------------*/
    struct {
      // dw_tag_command Type; // Type data TAG
      uint8_t Type;    // Type data TAG
      uint8_t Battery; // Battery
      uint8_t Motion;  // Motion
      uint8_t Button;  // Button
      union {
        uint32_t U32_Init[3];
        struct {
          uint32_t Compass;
          uint32_t Pressure;
          uint32_t Acceleration;
        } ETAG; /* Du lieu tuy bien cho khach hang */
        struct {
          uint8_t Temper;
          uint8_t Humi;
          uint8_t Vibra;
        } SOLUT; /* Du lieu tuy bien cho khach hang */
        struct {
          uint16_t Temper;
          uint32_t Pressure;
        } DPS422; /* Du lieu tuy bien cho cảm biến DPS422 */
        struct {
          uint16_t Version;
          uint16_t Temp;
          uint32_t Hi_MAC;
          uint32_t Low_MAC;
        } DUCTHANG; /* Du lieu kieu 5 */
      } Custom;
    } TAG; // 1+1+1+1+8*3 = 16bytes
  } Data;  // 12
  uint8_t Cmd;
  // dw_command Cmd;
  uint8_t TypeDev; // Type Device
  uint16_t DCRC;   // Luon du phong 2 byte CRC cho bo dem du lieu
} tag_frame_t;

/*─────────────────────────────────────────────*
 *         STRUCT GÓI DỮ LIỆU SERVER            *
 *─────────────────────────────────────────────*/
typedef struct {
  uint8_t Type_data;
  uint8_t Packit_ID[4];
  uint8_t Serial_ID[4];
  uint8_t Timestamp[5];
  struct {
    uint8_t Serial[4];
    uint8_t Timestamp[8];
    uint8_t Packit_ID[4];
  } Mts_access[DECAWAVE_MASTER_ACCESS_NUM];
  uint8_t Tag_ID[4];
  uint8_t Motion, Button, Free_fall, RSSI;

  union {
    struct {
      uint8_t buff_null[12];
    } Type0;
    struct {
      uint8_t Compass[4], Pressure[4], Accelermeter[4];
    } Type1;
    struct {
      uint8_t Temperature, Humidity, Vibrate;
    } Type2;
    struct {
      uint8_t Temperature[2], Pressure[4];
    } Type3;
    struct {
      uint8_t Version[2]; // 2 bytes
      uint8_t Temp[2];    // 2 bytes
      uint8_t Hi_MAC[4];  // 4 bytes
      uint8_t Low_MAC[4]; // 4 bytes
    } Type5;
  };
} server_dataframe_t;

/*─────────────────────────────────────────────*
 *       TDOA SERVER FIFO – CHỐNG GHI ĐÈ       *
 *─────────────────────────────────────────────*/
#ifndef TDOA_SERVER_QUEUE_SIZE
#define TDOA_SERVER_QUEUE_SIZE 64
#endif
typedef struct {
  server_dataframe_t frames[TDOA_SERVER_QUEUE_SIZE];
  volatile uint16_t read_index;
  volatile uint16_t write_index;
  volatile uint16_t count;
} tdoa_server_queue_t;

/*─────────────────────────────────────────────*
 *          STRUCT FRAME GIAO TIẾP DW3000       *
 *─────────────────────────────────────────────*/
typedef struct {
  uint32_t Des;
  uint32_t Src;
  volatile uint32_t Packet_Id;
  union {
    uint32_t U32_Init[4];
    struct {
      uint8_t Ts[5];
    } SYNC;
    struct {
      uint32_t Dis;
    } DIST;
    struct {
      struct {
        uint8_t poll_tx[5];
        uint8_t resp_rx[5];
        uint8_t final_tx[5];
      } TIME_STAMP;
      uint8_t Cmd;
    } DS_TWR;
    struct {
      uint8_t Type, Battery, Motion, Button;
      union {
        uint32_t U32_Init[3];
        struct {
          uint32_t Compass, Pressure, Acceleration;
        } ETAG;
        struct {
          uint8_t Temper, Humi, Vibra;
        } SOLUT;
        struct {
          uint16_t Temper;
          uint32_t Pressure;
        } DPS422;
        struct {
          uint16_t Version;
          uint16_t Temp;
          uint32_t Hi_MAC;
          uint32_t Low_MAC;
        } DUCTHANG;
      } Custom;
    } TAG;
  } Data;
  uint8_t Cmd;
  uint8_t TypeDev;
  uint16_t DCRC;
} dw_dataframe_t;

/*─────────────────────────────────────────────*
 *              STRUCT KHÁC                    *
 *─────────────────────────────────────────────*/
typedef struct {
  uint32_t Packet_Id;
  uint32_t Serial_ID;
} Old_Tag;

typedef struct {
  uint32_t Serial_ID;
  uint32_t Packet_ID;
} tag_tracking_t;

typedef struct {
  uint64_t poll_rx;
  uint64_t resp_tx;
  uint64_t final_rx;
} twr_local_timestamp_t;
extern twr_local_timestamp_t twr_ts_a; // Timestamp cục bộ của thiết bị bắt đầuu

typedef struct {
  uint64_t poll_tx;  // Thời điểm B gửi gói Poll
  uint64_t resp_rx;  // Thời điểm B nhận gói Resp từ A
  uint64_t final_tx; // Thời điểm B gửi gói Final về cho A
} twr_remote_timestamp_t;
extern twr_remote_timestamp_t twr_ts_b; // Timestamp phía responder

/*─────────────────────────────────────────────*
 *            BIẾN DÙNG CHUNG / EXTERN         *
 *─────────────────────────────────────────────*/
// extern bool has_tag_frame_ready;
extern uint32_t target_id; // ID của thiết bị mục tiêu (target)
extern bool flag_send_start_twr;
extern uint32_t MY_DEVICE_ID;
extern dw_mode_run mode_run;

extern volatile bool has_new_rx_frame;
extern volatile uint64_t last_rx_timestamp;
extern uint8_t last_rx_buf[RX_BUF_MAX_LEN];
extern uint16_t last_rx_len;

extern server_dataframe_t server_frame;
extern dw_dataframe_t last_dw_frame;
extern dwt_txconfig_t txconfig_options_ch9;

// static tag_tracking_t tag_cache[MAX_TAG_TRACKED] = {0};
// static uint8_t tag_cache_index = 0;

static uint8_t rx_buffer[RX_BUF_LEN];
static uint32_t status_reg = 0;
static server_dataframe_t latest_tag_frame;
static dw_dataframe_t twr_frame;
static uint64_t poll_rx_ts = 0;

/*─────────────────────────────────────────────*
 *         KHAI BÁO HÀM SỬ DỤNG NGOÀI          *
 *─────────────────────────────────────────────*/
uint8_t read_rssi_from_dw3000(void); // Đọc giá trị RSSI từ DW3000
void set_timestamp_u64_to_5bytes(uint64_t timestamp, uint8_t ts[5]);
uint8_t begin_dw3000(void);                              // Khởi tạo DW3000
void poll_rx_once(void);                                 // Nhận gói tin DW3000
void test_run_info(unsigned char *data);                 // In thông tin debug
int dw3000_tx_frame(uint8_t *buffer, uint16_t len);      // gửi gói tin offset
void print_dw_offset_frame(const dw_dataframe_t *frame); // in gói tin offset rx
uint64_t parse_ts_40bit(const uint8_t ts[5]);

void build_server_frame(
    const tag_frame_t *tag, uint64_t rx_ts, uint8_t rssi,
    server_dataframe_t *out); // Xây dựng gói dữ liệu server từ gói tag
bool tdoa_queue_push(const server_dataframe_t *frame);
bool tdoa_prepare_spi_frame(void);
void tdoa_update_irq(void);
int16_t calculate_rssi_dw3000(
    const dwt_rxdiag_t *diag); // Tính toán RSSI từ thông tin nhận được
bool is_duplicate_tag(uint32_t packet_id,
                      uint32_t src); // Kiểm tra gói tag có trùng lặp không

void send_start_twr(
    uint32_t target_id); // Gửi gói Poll để bắt đầu quá trình TWR
void twr_start_once_rx(dw_dataframe_t *frame,
                       uint64_t rx_ts_start); // Bắt đầu quá trình TWR
void twr_poll_once_rx(dw_dataframe_t *frame,
                      uint64_t rx_ts); // Poll trong quá trình TWR
void twr_resp_once_rx(dw_dataframe_t *frame,
                      uint64_t rx_ts); // Resp trong quá trình TWR
void twr_final_once_rx(dw_dataframe_t *frame,
                       uint64_t rx_ts); // Final trong quá trình TWR
// void twr_distance_once_rx(dw_dataframe_t *frame);              // Tính toán
// khoảng cách trong quá trình TWR

void tdoa_set_timestamp_u64(uint64_t Ts,
                            uint8_t *Dat); // Chuyển đổi timestamp sang 64-bit
uint64_t tdoa_get_timestamp_u64(
    const uint8_t *Dat); // Giải nén timestamp từ 5 byte sang 64-bit

void print_dw_dataframe(
    const dw_dataframe_t *frame); // In thông tin gói dữ liệu DW3000
void print_server_frame(
    const server_dataframe_t *f); // In thông tin gói dữ liệu server

uint32_t get_next_packet_id(void);    // Lấy ID gói tin tiếp theo
void handle_cmd_send_tx_offset(void); // Xử lý lệnh gửi gói tin Offset
#endif                                // DW3000_H