#ifndef _COM_REGS_H_
#define _COM_REGS_H_

// typedef enum
// {
//   led_error_DW = 0,
//   led_ok_DW,
//   led_sync_ERROR_DW,
//   led_clock_error_DW
// } status_ledDW;

// typedef enum
// {
//   led_internet_fail = 0,
//   led_internet_ok,
//   led_fail_server,
//   led_fail_TCP,
//   led_fail_MQTT
// } status_ledINTERNET;

/****************************CÁC BIẾN KHÔNG PHẢI LƯU
 * LẠI*********************************/
/*led status*/
typedef struct {
  uint8_t internet;
  uint8_t power;
  uint8_t decawave;
} handle_led_status_t;

/*xóa file last update*/
typedef struct {
  bool flag_del;
} handle_last_update_t;

/****************************CÁC BIẾN LƯU LẠI XỬ
 * LÝ*********************************/
/*address access*/
typedef struct {
  uint32_t device;
  uint32_t broadcast;
} serial_id_t;

/*thông số dành cho master*/
typedef struct {
  uint32_t interval;
  uint16_t time_prepare;
  bool enable;
} master_t;

/*master access*/
typedef struct {
  bool enable;
  uint32_t Serial;
  uint64_t Timestamp;
} master_access_t;

/*dw config*/
typedef struct {
  uint8_t chan;
  uint8_t prf;
  uint8_t txPreambLength;
  uint8_t rxPAC;
  uint8_t txCode;
  uint8_t rxCode;
  uint8_t nsSFD;
  uint8_t dataRate;
  uint8_t phrMode;
  uint16_t sfdTO;
} dw_config_t;

/*dw config tx*/
typedef struct {
  uint8_t PGdly;
  uint32_t power;
} dw_txconfig_t;

/*anten delay*/
typedef struct {
  uint16_t rx;
  uint16_t tx;
} anten_delay_t;

/*dw config tx*/

typedef struct {
  uint32_t deviceID;
  uint32_t distance;
} dw_two_way_t;

/*last update*/
typedef struct {
  uint32_t unit_time;
} last_update_t;

typedef struct {
  uint16_t size_data;
  uint8_t data[2048];
} write_ota_t;

typedef struct {
  uint16_t timeout;
  uint8_t reset;
} reset_t;

typedef struct {
  uint8_t FW_VR[10];
  uint8_t HW_VR[10];
} version_t;

typedef struct {
  uint64_t status;
} fragment_status_t;

// ---- Struct Kết quả Broadcast TWR (Tag ID + Khoảng cách) ----
typedef struct {
  uint8_t  tag_id[5];   // ID Tag đo được (5 bytes)
  uint32_t distance_mm; // Khoảng cách (milimet), 0xFFFFFFFF = lỗi
} __attribute__((packed)) dw_twr_result_t;

// Biến enable chế độ Broadcast TWR
extern bool enable_broadcast_twr;

// ---- Struct Request Data từ ISP3080 ----
typedef struct {
  uint8_t send_tag_motion, send_tag_stand, send_tag_sleep1, send_tag_sleep2,
      send_tag_sleep3;
  uint8_t sleep_mode1, sleep_mode2, sleep_mode3;
  uint8_t batt_default, batt_high, batt_increase, batt_decrease;
} __attribute__((packed)) request_timer_t;

typedef struct {
  uint8_t uwb_chan, uwb_plen, uwb_pac, uwb_txcode, uwb_rxcode, uwb_sfdtype,
      uwb_datarate, uwb_phrmode, uwb_phrrate;
  uint16_t uwb_sfdto;
  uint8_t uwb_stsmode, uwb_stslen, uwb_pdoa;
} __attribute__((packed)) request_config_t;

typedef struct {
  uint8_t id[5]; // ID Tag (5 bytes)
  // uint8_t mac[6]; // MAC Address (6 bytes)
} __attribute__((packed)) request_id_tag_t;

typedef struct {
  request_timer_t timer;
  request_config_t config;
  uint32_t current_state;
  request_id_tag_t id_tag;
} __attribute__((packed)) request_data_t;

// --- Thêm struct beacon_cfg_t (Copy từ U6) ---
typedef struct {
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
  uint8_t val_request;   // Gói yêu cầu (0: none, 1: timer, 2: config, 3: state,
                         // 4: id)
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
  uint8_t apply_config_to_base; // 1: Ap dung cho Base sau khi phat xong, 0: Chi
                                // phat qua BLE

  // --- MAC Address (5 bytes = 10 hex chars) ---
  uint8_t mac_address[5]; // MAC address của thiết bị (VD: "1023ABCDEF")

  // --- Broadcast Mode ---
  uint8_t broadcast; // 1: Broadcast cho tất cả Tag (không ACK, 30s/fragment), 0: Unicast
  uint8_t enable_bcast_twr; // 1: Enable auto-scanning and sequential TWR measuring

  // --- Setting (0x40) ---
  uint8_t charge_tx; // 1: OFF, 2: ON
  uint8_t ota_enable; // 1: ENABLE OTA
  uint8_t sys_config_default; // 1: reset tag SYS_CONFIG về mặc định
  uint8_t sleep_enable; // 1: DISABLE SLEEP, 2: ENABLE SLEEP
} __attribute__((packed)) beacon_cfg_t;

extern beacon_cfg_t g_beacon_cfg;
/****************************KIỂU DỮ LIỆU CHO VIỆC GIAO
 * TIẾP*********************************/
/* header đọc hoặc ghi*/
typedef enum {
  read_config = 0x01,
  write_config = 0x02,
  read_ram = 0x03,
  write_ram = 0x04,
  infor_ota = 0x05,
  write_ota = 0x06,
  read_distant = 0x07,
  read_twr_result = 0x08, // Kết quả Broadcast TWR (Tag ID + Distance)
} type_com_t;

/*mask registers*/
typedef struct {
  uint16_t add;
  uint16_t len;
} mask_regs_t;

/****************************CÁC BIẾN KHÔNG PHẢI LƯU
 * LẠI*********************************/
/**
 * địa chỉ xử lý trên RAM: từ 0x1000-0x2FFF
 * type data thì chỉ sử dụng: read_ram và write_ram
 * liên quan tới type read_ram và write_ram thì "handle_spifs" sẽ xử lý
 */
const mask_regs_t HANDLE_LED_STATUS_RES = {
    0x1000, sizeof(handle_led_status_t)}; // địa chỉ của trạng thái
const mask_regs_t HANDLE_LAST_UPDATE_RES = {
    0x1001,
    sizeof(handle_last_update_t)}; // địa chỉ của ra lệnh xóa file config
const mask_regs_t SET_TWO_WAY = {0x1002, sizeof(dw_two_way_t)}; // read twoway
const mask_regs_t START_OTA = {0x1003, sizeof(write_ota_t)};    // ota
const mask_regs_t START_RESET = {0x1004, sizeof(reset_t)};      // RESET
const mask_regs_t READ_VERSION = {0x1005, sizeof(version_t)};   // VERSION
const mask_regs_t FRAGMENT_STATUS_RES = {
    0x1006, sizeof(fragment_status_t)}; // fragment status
const mask_regs_t REQUEST_DATA_RES = {
    0x1007, sizeof(request_data_t)}; // request data status
const mask_regs_t TWR_RESULT_RES = {
    0x1008, sizeof(dw_twr_result_t)}; // broadcast twr result

/****************************CÁC BIẾN LƯU LẠI XỬ
 * LÝ*********************************/
/**
 * địa chỉ lưu lại phân vùng spifs: từ 0x3000-0xFFFF
 * type data thì chỉ sử dụng: read_config và write_config
 * liên quan tới type read_config và write_config thì "handle_com" sẽ xử lý
 */
const mask_regs_t SERIAL_ID_RES = {0x3000, sizeof(serial_id_t)}; // địa chỉ base
const mask_regs_t MASTER_RES = {
    0x3001, sizeof(master_t)}; // địa chỉ nhận và phát tất cả

const mask_regs_t MASTER_ACCESS_RES1 = {
    0x3002, sizeof(master_access_t)}; // địa chỉ nhận sync 1
const mask_regs_t MASTER_ACCESS_RES2 = {
    0x3003, sizeof(master_access_t)}; // địa chỉ nhận sync 2
const mask_regs_t MASTER_ACCESS_RES3 = {
    0x3004, sizeof(master_access_t)}; // địa chỉ nhận sync 3
const mask_regs_t MASTER_ACCESS_RES4 = {
    0x3005, sizeof(master_access_t)}; // địa chỉ nhận sync 4

const mask_regs_t DW_CONFIG_RES = {0x3006,
                                   sizeof(dw_config_t)}; // decawave config
const mask_regs_t DW_CONFIG_TX_RES = {
    0x3007, sizeof(dw_txconfig_t)}; // decawave config phát
const mask_regs_t DW_ANT_DELAY_RES = {
    0x3008, sizeof(anten_delay_t)}; // anten delay phát
const mask_regs_t LAST_UPDATE_RES = {0x3009,
                                     sizeof(last_update_t)}; // last update
const mask_regs_t BEACON_CFG_RES = {0x300A,
                                    sizeof(beacon_cfg_t)}; // beacon config

#endif
