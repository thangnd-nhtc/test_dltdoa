/*define version*/
// #define D_HARDWARE_VERSION "1.10"
// #define D_FIRMWARE_VERSION "1.30"

#define I_HARDWARE_VERSION "2.00"
// #define I_FIRMWARE_VERSION "2.19"
#define I_FIRMWARE_VERSION "2.20"

#define INDENTIFY_CODE "I"
// #define BEACON_UUID "27137124-3da6-4cce-ad72-f18e8ddbef86"
/*define pin*/
// enable clock ethernet
#define ETH_CLK_EN 5
// io
#define BUTTON_BOOT 0
// #define BUZZER_BEEP 32
// spi com
#define SPI_COM_CLK 14
#define SPI_COM_MOSI 13
#define SPI_COM_MISO 12
#define SPI_COM_CS 4
#define SPI_COM_DRDY 2 // new=2
// sd card
#define SD_CARD_POWER 15
#define SD_CARD_CS 33

// uart
#define UART_TX 32
#define UART_RX 34 // new=34

/*define communication*/
#define SPI_COM_BUFFER_NUM 5
#define SPI_COM_BUFFER_LENGTH 100

// file config
// #define FILE_CONFIG_DEVICE "config_device.json"
// #define FILE_CONFIG_DECAWAVE "config_decawave.json"
// #define FILE_CONFIG_INTERNET "config_internet.json"
// #define FILE_CONFIG_SPI_COM "config_spi_com.json"

/*define debug*/
#define EEPROM_ADDR_DEBUG 120 // dời cờ debug sang vùng cuối, tránh 0..41

// select program print debug
#define DEBUG_MAIN 0
#define DEBUG_MQTT 0
#define DEBUG_SD 0
#define DEBUG_ETH 1
#define DEBUG_WIFI 0
#define DEBUG_OTA 1
#define DEBUG_TCP 0
#define DEBUG_SPI 0
#define DEBUG_WEBSERVER 0
#define DEBUG_SPIFFS 0
#define DEBUG_FTP 0
#define DEBUG_BEACON 0

#define DEBUG_CMD1 0
#define DEBUG_CMD2 0
// uart printf debug
// #include "HardwareSerial.h"
#define DEBUG_COM Serial
// program print debug
#define dbg_main(fmt, ...) \
  (DEBUG_MAIN) ? DEBUG_COM.printf(PSTR("\r\n>Main< " fmt), ##__VA_ARGS__) : NULL
#define dbg_mqtt(fmt, ...) \
  (DEBUG_MQTT) ? DEBUG_COM.printf(PSTR("\r\n>MQTT< " fmt), ##__VA_ARGS__) : NULL
#define dbg_sd(fmt, ...) \
  (DEBUG_SD) ? DEBUG_COM.printf(PSTR("\r\n>SD< " fmt), ##__VA_ARGS__) : NULL
#define dbg_enthernet(fmt, ...) \
  (DEBUG_ETH) ? DEBUG_COM.printf(PSTR("\r\n>ETH< " fmt), ##__VA_ARGS__) : NULL
#define dbg_wifi(fmt, ...) \
  (DEBUG_WIFI) ? DEBUG_COM.printf(PSTR("\r\n>WIFI< " fmt), ##__VA_ARGS__) : NULL
#define debug_OTA(fmt, ...) \
  (DEBUG_OTA) ? DEBUG_COM.printf(PSTR("\r\n>OTA< " fmt), ##__VA_ARGS__) : NULL
// #define debug_mqtt(fmt, ...) (DEBUG_MQTT) ? DEBUG_COM.printf(PSTR("\r\n>MQTT<
// " fmt), ##__VA_ARGS__) : NULL
#define debug_TCP(fmt, ...) \
  (DEBUG_TCP) ? DEBUG_COM.printf(PSTR("\r\n>TCP< " fmt), ##__VA_ARGS__) : NULL
#define debug_SPI(fmt, ...) \
  (DEBUG_SPI) ? DEBUG_COM.printf(PSTR("\r\n>SPI< " fmt), ##__VA_ARGS__) : NULL
#define debug_webserver(fmt, ...)                                      \
  (DEBUG_WEBSERVER)                                                    \
      ? DEBUG_COM.printf(PSTR("\r\n>WEB_SERVER< " fmt), ##__VA_ARGS__) \
      : NULL
#define debug_SPIFFS(fmt, ...)                                                \
  (DEBUG_SPIFFS) ? DEBUG_COM.printf(PSTR("\r\n>SPIFFS< " fmt), ##__VA_ARGS__) \
                 : NULL
#define debug_ftp(fmt, ...) \
  (DEBUG_FTP) ? DEBUG_COM.printf(PSTR("\r\n>FTP< " fmt), ##__VA_ARGS__) : NULL
#define debug_beacon(fmt, ...)                                                \
  (DEBUG_BEACON) ? DEBUG_COM.printf(PSTR("\r\n>BEACON< " fmt), ##__VA_ARGS__) \
                 : NULL

#define cmd1_debug(fmt, ...) \
  (DEBUG_CMD1) ? DEBUG_COM.printf(PSTR("\r\n>CMD1< " fmt), ##__VA_ARGS__) : NULL
#define cmd2_debug(fmt, ...) \
  (DEBUG_CMD2) ? DEBUG_COM.printf(PSTR("\r\n>CMD2< " fmt), ##__VA_ARGS__) : NULL
/*ethenet*/
#define DEFINE_ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
#define DEFINE_ETH_POWER_PIN -1
#define DEFINE_ETH_TYPE ETH_PHY_LAN8720
#define DEFINE_ETH_ADDR 1
#define DEFINE_ETH_MDC_PIN 23
#define DEFINE_ETH_MDIO_PIN 18
