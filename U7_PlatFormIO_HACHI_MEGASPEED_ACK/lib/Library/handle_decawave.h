#ifndef _HANDLE_DW_H_
#define _HANDLE_DW_H_

#include "Arduino.h"
#include "TimeOutEvent.h"

#include <functional>

#include "ESP_aes.h"
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"

#include "define.h"
#include "handle_com.h"
#include "handle_spifs.h"
#include "handle_status.h"

/* Dinh nghia thoi gian Sync cua Master */
/* 0x3CF00		1ms
 * 0x79E00		2ms
 * 0x130B00		5ms
 * 0xBE6E00		50ms
 * 0x17CDC00	100ms
 * */
#define TIME_SYNC_1MS (0x3CF00)

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) conversion
 * factor. 1 uus = 512 / 499.2 and 1 = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536

/* Speed of light in air, in metres per second. */
#define SPEED_OF_LIGHT 299792458 // 299702547

typedef enum {
  Cmd_start_twr = 0, // bắt đầu đo
  Cmd_Pool = 1,      // bắt đầu đo twr
  Cmd_Resp = 2,      // trả lời lệnh đo twr
  Cmd_Final = 3,     // tính được khoảng cách
  Cmd_Distance = 4,  // gửi khoảng cách cho đối diện
  Cmd_Sync = 5,      // động bộ clock
  Cmd_Tag = 7,       // dữ liệu của tag
  Cmd_Offset = 8
} dw_command;

typedef enum {
  Cmd_tag_nomal = 0,
  Cmd_tag_sensor = 1, // TAG cho nhan vien
  Cmd_tag_solut = 2,  // TAG cho moi truong
  Cmd_tag_hoya = 3,   // TAG cho HOYA
  Cmd_tag_dps422 = 4  // TAG cho DPS422
} dw_tag_command;

typedef struct {
  uint32_t Packet_Id; // Packet_Id
  uint32_t Serial_ID;
} Old_Tag;

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
        } DUCTHANG;
      } Custom;
    } TAG; // 1+1+1+1+8*3 = 16bytes
  } Data;  // 12
  uint8_t Cmd;
  // dw_command Cmd;
  uint8_t TypeDev; // Type Device
  uint16_t DCRC;   // Luon du phong 2 byte CRC cho bo dem du lieu
} dw_dataframe_t;  // 12 + 16 + 4 = 32
#define SIZE_OF_DATAFRAME sizeof(dw_dataframe_t)

typedef enum {
  mode_wait,
  mode_tdoa,
  mode_twr,
  mode_sync_tx,
  mode_sync_rx,
  mode_offset_tx,
  mode_offset_rx
} dw_mode_run;

// frame data gửi dữ liệu lên server
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
  uint8_t Motion;
  uint8_t Button;
  uint8_t Free_fall;
  uint8_t RSSI;
  union {
    struct // nomal
    {
      uint8_t buff_null[12];
    } Type0;
    struct // Cmd_tag_sensor
    {
      uint8_t Compass[4];
      uint8_t Pressure[4];
      uint8_t Accelermeter[4];
    } Type1;
    struct // Cmd_tag_solut
    {
      uint8_t Temperature;
      uint8_t Humidity;
      uint8_t Vibrate;
    } Type2;
    struct // Cmd_tag_dps422
    {
      uint8_t Temperature[2];
      uint8_t Pressure[4];
    } Type3;
    struct // Cmd_tag_ducthang
    {
      uint8_t Version[2];
      uint8_t Temp[2];
      uint8_t Hi_MAC[4];
      uint8_t Low_MAC[4];
    } Type5;
  };
} server_dataframe_t;

extern bool FlagReadconfig;

class DwHandle {
public:
  DwHandle(/* args */);
  ~DwHandle();
  typedef std::function<void(double)> HandlerFunction;

  // init
  bool begin(void);
  // void creatfileDW();
  void reloadDW();
  void twr_start(uint32_t addr, HandlerFunction handler); // 35708
  // reciver
  void reciver(void);
  // handle
  void tdoa(void);
  void sync_rx(void);
  void sync_tx(void);
  void tx_offset(void);
  void handle_ducthang_twr();
  void offset_rx(void);
  void twr(void);
  void recheck(void);
  void checkResetDW();
  void sync_DW(void);
  void check_isp3080(void);
  bool begin_nrf(void);

private:
  bool decawace_ready_f = false;
  TimeOutEvent *decawave_recheck_to;

  dw_mode_run mode;
  bool reciver_en_flag = true;

  struct {
    uint64_t nomal;
    uint64_t min;
    uint64_t max;
  } interval_sync;

  /*Tính twr*/
  struct {
    bool flag;
    uint32_t distance;
    dw_dataframe_t data;
  } toway;

  /*data reciver raw*/
  struct {
    uint32_t length;
    uint8_t buffer[SIZE_OF_DATAFRAME + 8];
    uint64_t tx_timestamp;
    uint64_t rx_timestamp;
    dwt_rxdiag_t rx_diag;
    uint16_t Pream_AccCnt; // Preamble Accumulation Count
  } data_rx_raw;

  /*data reciver*/
  dw_dataframe_t data_rx[2];
  dw_dataframe_t *frame_tag = data_rx;    // dữ liệu theo tag
  dw_dataframe_t *frame_sync = data_rx;   // dữ liệu theo sync
  dw_dataframe_t *frame_offset = data_rx; // dữ liệu theo offset
  dw_dataframe_t frame_offset1;
  /*frame data gửi lên server*/
  server_dataframe_t data_transmits;

  // load parameters to config decawave
  void parameters(void);

  // double buffer
  void Buffer_En(void);
  void Buffer_Index(void);

  // check tranmits
  bool Check_TX(int status);

  // recheck
  void set_recheck(void);
  void set_recheck_twr(void);

  // Read buffer
  void reciver_enable(bool status);
  int8_t reciver_data_raw(uint8_t *data, uint32_t threshold_length,
                          uint8_t rd_tx_ts, uint8_t rd_rxts);

  // calculator timestamp
  uint64_t tdoa_get_timestamp_u64(uint8_t *Dat);
  void tdoa_set_timestamp_u64(uint64_t Ts, uint8_t *Dat);
  uint64_t get_tx_timestamp_u64(void);
  uint64_t get_rx_timestamp_u64(void);

  uint8_t value2array(uint8_t *Array, uint64_t Value, uint8_t Length);

  HandlerFunction toway_callback = NULL;
};

extern DwHandle Handle_Dw;

extern bool is_sending_config;
extern uint32_t last_quiet_ms;

#endif
