/*define version*/
#define HARDWARE_VERSION "2.01"
// #define FIRMWARE_VERSION "2.38"
#define FIRMWARE_VERSION "2.38"
#define INDENTIFY_CODE "D"
/*define pin*/
#define RESET_CLKDW 21
// io
#define BUTTON_BOOT 0
#define LED1_GRN 17
#define LED2_GRN 2  // 2
#define LED2_RED 4  // 4
#define LED3_GRN 33 // 33
#define LED3_RED 32 // 32
// i2c
//  #define I2C_SCL 25
//  #define I2C_SDA 26
// spi dw
#define SPI_DW_CLK 14
#define SPI_DW_MOSI 13
#define SPI_DW_MISO 12
#define SPI_DW_CS 15
// dw
#define DW_IRQ 35
#define DW_RESET 27
#define DW_CLK_CS 16
// spi com
#define SPI_COM_CLK 18
#define SPI_COM_MOSI 23
#define SPI_COM_MISO 19
#define SPI_COM_CS 5
// #define SPI_COM_DRDY 22
// uart
#define UART_TX1
#define UART_RX1
// SENSOR DW
#define LOL 36
#define LOS 39

/*define decawave*/
#define DECAWAVE_SYNC_WIRE 1
#define DECAWAVE_SYNC_AIR 0
#define DECAWAVE_SELECT_CLOCK 0 // 0:extenal, 1:internal
#define DECAWAVE_MASTER_ACCESS_NUM 4

/*define communication*/
#define COMMUNICATION_NUM_BUFFER 5
#define COMMUNICATION_LENGHT_BUFFER 256

/*SPIFS cho prametter*/
// #define FILE_PARAMETTER_DW "/config_decawave.json"

/*define debug*/
// select program print debug
#define DEBUG_MAIN 0
#define DEBUG_TDoA 0
#define DEBUG_DW 0
#define DEBUG_DW_RX 0
#define DEBUG_DW_TDOA 0

#define DEBUG_DW_SYNC 0
#define DEBUG_DW_TWR 0
#define DEBUG_AES 0
#define DEBUG_COM 0
#define DEBUG_SPIFS 0
#define DEBUG_TCP 0
#define DEBUG_SPI 0
// uart printf debug
#include "HardwareSerial.h"
#define SERIAL_DEBUG Serial
// program print debug
#define dbg_main(fmt, ...) \
  (DEBUG_MAIN) ? SERIAL_DEBUG.printf(PSTR("\r\n>Main< " fmt), ##__VA_ARGS__) : 0
#define dbg_dw(fmt, ...) \
  (DEBUG_DW) ? SERIAL_DEBUG.printf(PSTR("\r\n>DW< " fmt), ##__VA_ARGS__) : 0
#define dbg_dw_rx(fmt, ...)                                                    \
  (DEBUG_DW_RX) ? SERIAL_DEBUG.printf(PSTR("\r\n>DW RX< " fmt), ##__VA_ARGS__) \
                : 0
#define dbg_dw_tdoa(fmt, ...)                                          \
  (DEBUG_DW_TDOA)                                                      \
      ? SERIAL_DEBUG.printf(PSTR("\r\n>DW TDOA< " fmt), ##__VA_ARGS__) \
      : 0
#define dbg_TDoA(fmt, ...) \
  (DEBUG_TDoA) ? SERIAL_DEBUG.printf(PSTR("\r\n>TDoA< " fmt), ##__VA_ARGS__) : 0
#define dbg_dw_sync(fmt, ...)                                          \
  (DEBUG_DW_SYNC)                                                      \
      ? SERIAL_DEBUG.printf(PSTR("\r\n>DW SYNC< " fmt), ##__VA_ARGS__) \
      : 0
#define dbg_dw_twr(fmt, ...)                                          \
  (DEBUG_DW_TWR)                                                      \
      ? SERIAL_DEBUG.printf(PSTR("\r\n>DW TWR< " fmt), ##__VA_ARGS__) \
      : 0
#define dbg_aes(fmt, ...) \
  (DEBUG_AES) ? SERIAL_DEBUG.printf(PSTR("\r\n>AES< " fmt), ##__VA_ARGS__) : 0
#define dbg_com(fmt, ...) \
  (DEBUG_COM) ? SERIAL_DEBUG.printf(PSTR("\r\n>COM< " fmt), ##__VA_ARGS__) : 0
#define dbg_spifs(fmt, ...)                                                    \
  (DEBUG_SPIFS) ? SERIAL_DEBUG.printf(PSTR("\r\n>SPIFS< " fmt), ##__VA_ARGS__) \
                : 0
#define dbg_tcp(fmt, ...) \
  (DEBUG_TCP) ? SERIAL_DEBUG.printf(PSTR("\r\n>TCP< " fmt), ##__VA_ARGS__) : 0
#define dbg_spi(fmt, ...) \
  (DEBUG_SPI) ? SERIAL_DEBUG.printf(PSTR("\r\n>SPI< " fmt), ##__VA_ARGS__) : 0
