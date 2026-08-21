#ifndef _MQTT_DATA_BASE_H
#define _MQTT_DATA_BASE_H

#include "Arduino.h"
#include "stdint.h"
#include <ArduinoJson.h>

// #include "FileConfig.h"
// #include "Log_File.h"
// #include "timeout.h"
// #include "Handle_Time.h"
// #include "Handle_Audio.h"
#include "handle_com_regs.h"
#include "handle_sdcard.h"

#define OTSD_Firmware_Main "firmware_main.bin"
#define OTSD_Firmware_LED "firmware_dw.bin"

/*MQTT*/
#define MQTT_CMD_TOKEN_KEY 1  // Token key
#define MQTT_CMD_UPDATE_OTA 2 // Update OTA
#define MQTT_CMD_RESET_ALL 3  // reset
// #define MQTT_CMD_RESET_ALL 				4	  //reset DW
#define MQTT_CMD_SERVER_DATA 4      // server data
#define MQTT_CMD_SERVER_MQTT 5      // server mqtt
#define MQTT_CMD_SERVER_FTP 6       // server FTP
#define MQTT_CMD_SERVER_NTP 7       // server NTP
#define MQTT_CMD_TWO_WAY 8          // two way
#define MQTT_CMD_CONFIG_DW 9        // channel dw
#define MQTT_CMD_SET_MASTER 10      // set master
#define MQTT_CMD_ACC_SYNC_MASTER 11 // accept sync mater
#define MQTT_CMD_STATUS_INTERNET 12 // status internet
#define MQTT_CMD_STATUS_DECAWAY 13  // status decaway
#define MQTT_CMD_STATUS_BEACON 14   // status beacon
#define MQTT_CMD_STATUS_HW 15       // status hardware
#define MQTT_CMD_BEACON 16          // status beacon
#define MQTT_CMD_BEACON_CONFIG 20   // dynamic beacon config
// CMD 21 đã chuyển sang TCP port 2011, không dùng MQTT nữa
#define MQTT_CMD_ISP_VERSION 24     // ISP3080 firmware version
#define MQTT_CMD_CONFIG_NET 25      // network config (MAC, HostName, ID)

/*Error code*/

#define HW_SD_card_not_found "E2"
#define HW_SD_card_is_read "SD_IS_READY"
#define HW_SD_card_full_memory "E3"
#define HW_SD_card_memory_nomal "SD_MEMORY_NOMAL"

#define OTA_Ver_firmware_null "E5"
#define OTA_Ver_firmware_old "E6"
#define OTA_Ver_hardware_null "E7"
#define OTA_Ver_hardware_old "E8"
#define OTA_Board_update_not_found "E9"
#define OTA_Path_FTP_not_found "E10"
#define OTA_MD5_compare_fail "E11"
#define OTA_File_not_found "E12"
#define OTA_update_fail "E13"
#define OTA_update_ok "OTA_UPDATE_OK"
#define OTA_MD5_Error "E14"

/*header*/
#define MQTT_RANDOM "Random"      // biến random
#define MQTT_SERIAL "SerialID"    // Mã thiết bị
#define MQTT_CMD "CMD"            // Lệnh thực thi
#define MQTT_PACKIT_ID "PackitID" // ID của gói tin
#define MQTT_REPLY "Reply"        // trả lời

/*cmd 1*/
#define MQTT_DEFAULT_KEY "defaultkey" // Token key mặt định
#define MQTT_TOKEN_KEY "TokenKey"     // Token key cần trao đổi
#define MQTT_HW_INTERNET "HwIT"       // hardware Internet
#define MQTT_FW_INTERNET "FwIT"       // firmware Internet
#define MQTT_HW_DW "HwDW"             // hardware Decawave
#define MQTT_FW_DW "FwDW"             // firmware Decawave

/*cmd 2*/
#define MQTT_MD5 "MD5"          // Key để giao tiếp các lệnh khác
#define MQTT_HW_VERSION "HwVer" // hardware Version
#define MQTT_FW_VERSION "FwVer" // firmware Version
#define MQTT_FILE                                                              \
  "FileName" // nRTLS3_D_H1.00_F1.01.bin (D: cho chip decawave, I: cho chip
             // internet)
#define MQTT_PATH "Path" // Thư mục chứa file trên FTP

/*cmd 4-5-6-7*/
#define MQTT_SERVER "Server"            // link server: nrtls.com.vn
#define MQTT_PORT "Port"                // Port
#define MQTT_USER "User"                // User để đăng nhập vào MQTT
#define MQTT_PASS "Pass"                // Mật khẩu để vào MQTT
#define MQTT_TOPIC_SERVER "TopicServer" // topic server
#define MQTT_TOPIC_DEVICE "TopicDevice" // topic device

/*cmd 8-9-10-11*/
#define MQTT_BASE_ID "FileID"       // Thứ tự file trả về trong list danh sach
#define MQTT_DISTANCE_ID "FileName" // File cần thao tác
#define MQTT_CHANNEL "Channle"      //
#define MQTT_PREAMBLE "Preamble"    //
#define MQTT_PG_DELAY "PGDelay"     //
#define MQTT_TX_POWER "TXPower"     //
#define MQTT_SYNC "Sync"            //
#define MQTT_PERPARE "Perpare"      //
#define MQTT_ACCEPT_1 "Accept1"     //
#define MQTT_ACCEPT_2 "Accept2"     //
#define MQTT_ACCEPT_3 "Accept3"     //
#define MQTT_ACCEPT_4 "Accept4"     //

/*cmd 12-13-14-15*/
#define MQTT_LAN "Lan"           //
#define MQTT_IP_LAN "IPLan"      //
#define MQTT_WIFI "Wifi"         //
#define MQTT_IP_WIFI "IPWifi"    //
#define MQTT_CONNCET "Connect"   //
#define MQTT_MASTER "Master"     //
#define MQTT_INTERVAL "Interval" //
#define MQTT_TYPE "Type"         //
#define MQTT_CHIP_DW "ChipDW"    //
#define MQTT_SD_CRAD "SdCard"    //

/**/
#define MQTT_TIME_REPORT 5150
#define MQTT_TIME_PREPARE 200
/*size*/
#define L_SERIAL_SIZE 20  // độ dài của serial number
#define L_VERSION_SIZE 10 // độ dài của chuỗi version
#define L_PATH_SIZE 100
#define L_MD5_SIZE 32 // độ dài của chuỗi check sum MD5
#define REPLY_SIZE 5  // độ dài của chuỗi trả lời
/*length cmd1*/
#define L_TOKEN_KEY_SIZE 16 // độ dài của chuỗi token key
#define L_HW_INTERNET 4     // độ dài của chuổi ver harware internet
#define L_FW_INTERNET 4     // độ dài của chuổi ver harware internet
#define L_HW_DW 4           // độ dài của chuổi ver harware internet
#define L_FW_DW 4           // độ dài của chuổi ver harware internet
/*length cmd2*/
#define L_MD5 100
#define L_HW 4
#define L_FW 4
#define L_FILE 100
#define L_PATH 100
/*length cmd4-5-6-7*/
#define L_SERVER 100
#define L_USER_MQTT 100
#define L_PASS_MQTT 100
#define L_TOPIC_SERVER 100
#define L_TOPIC_DEVICE 100
#define L_SERVER_FTP 100
#define L_SERVER_NTP 100
/*length cmd8-9-10-11*/
#define L_BASE_ID 8
#define L_ACCEPT 8
/*length cmd12-13-14-15*/
#define L_IPLAN 100
#define L_IPWIFI 100

// extern void scream_config();

// extern TimeOutEvent ESPRebootTo;

/*CMD 1*/
typedef struct {
  char TokenKey[L_TOKEN_KEY_SIZE + 1];
  char HWInternet[L_HW_INTERNET + 1];
  char FWInternet[L_FW_INTERNET + 1];
  char HWDecawave[L_HW_DW + 1];
  char FWDecawave[L_FW_DW + 1];
} cmd1_str;
extern cmd1_str _cmd1_str;

/*CMD 2*/
typedef struct {
  char MD5[L_MD5 + 1];
  char Hardware[L_HW + 1];
  char Firmware[L_FW + 1];
  char FileName[L_FILE + 1];
  char Path[L_PATH + 1];
} cmd2_str;

typedef struct {
  uint8_t Reset_status;
} cmd3_str;

/*CMD 4 & 5 & 6 & 7*/
typedef struct {
  /*cmd 4*/
  char Server[L_SERVER + 1];
  uint16_t PortServer;
  /*cmd 5*/
  char ServerMqtt[L_SERVER + 1];
  uint16_t PortMqtt;
  char UserMqtt[L_USER_MQTT + 1];
  char PassMqtt[L_PASS_MQTT + 1];
  char TopicServerMqtt[L_TOPIC_SERVER + 1];
  char TopicDevicerMqtt[L_TOPIC_DEVICE + 1];
  /*cmd 6*/
  char ServerFTP[L_SERVER_FTP + 1];
  uint16_t PortFTP;
  /*cmd 7*/
  char ServerNTP[L_SERVER_NTP + 1];
  uint16_t PortNTP;
} cmd4_5_6_7_str;

/*CMD 8 & 9 & 10 & 11*/
typedef struct {
  /*cmd 8*/
  uint32_t BaseID;
  uint32_t Distance;
  /*cmd 9*/
  uint8_t Channel;
  uint8_t Preamble;
  uint8_t PGdelay;
  uint8_t TxPower;
  /*cmd 10*/
  uint16_t Sync;
  uint16_t Prepare;
  /*cmd 11*/
  char Accept1[L_ACCEPT + 1];
  char Accept2[L_ACCEPT + 1];
  char Accept3[L_ACCEPT + 1];
  char Accept4[L_ACCEPT + 1];
} cmd8_9_10_11_str;

/*CMD 12 & 13 & 14 & 15*/
typedef struct {
  /*cmd 12*/
  bool Lan;
  char IPLan[L_IPLAN + 1];
  bool Wifi;
  char IPWifi[L_IPWIFI + 1];
  /*cmd 13*/
  bool Connect;
  bool Master;
  /*cmd 14*/
  uint16_t Interval;
  uint8_t Type;
  /*cmd 15*/
  bool Chipdw;
  bool Sdcard;
} cmd12_13_14_15_str;

typedef struct {
  uint16_t Random;
  uint32_t SerialID; // ID thiết bị
  uint8_t CMD;
  uint16_t CMDServerID;
  uint16_t PackitID;
  char Reply[REPLY_SIZE + 1];

  cmd1_str cmd1; // cmd1
  cmd2_str cmd2;
  cmd3_str cmd3;
  cmd8_9_10_11_str cmd8_9_10_11;
  // cmd_status_str cmd_status; //cmd 2
  // cmd3_4_str cmd3_4;		   //cmd 3_4
  // cmd5_str cmd5;			   //cmd 5
  // cmd6_str cmd6;			   //cmd 6
  // cmd8_9_str cmd8_9;		   //cmd 8_9
  // cmd10_str cmd10;		   //cmd 10
  // cmd12_str cmd12;		   //cmd 12
  // cmd13_str cmd13;		   //cmd 13
  // cmd17_str cmd17;		   //cmd 17
  // cmd19_str cmd19;		   //cmd 19

} MQTT_Exchange_str;
extern MQTT_Exchange_str MQTT_Exchange;
// extern cmd_status_str cmd2_pss;

// extern statusDevice _statusDevice;

// typedef enum
// {
// 	read_config = 0x01,
// 	write_config = 0x02,
// 	read_data = 0x03,
// 	write_data = 0x04,
// } type_com_t;

// typedef struct
// {
// 	uint16_t add;
// 	uint16_t len;
// } mask_regs_t;

typedef struct {
  struct {
    uint8_t type;
    uint16_t check_crc;
    mask_regs_t msk_regs;
  } header;
  uint8_t data[256];
} com_frame_t;

/*CMD 1*/
bool cmd1_is_tokenkey(void);
bool cmd1_info(cmd1_str *data);
void cmd1_resend_version(void);
void cmd1_lost_key(cmd1_str *data);
String cmd1_check(void);
String cmd1_decryption(char *AES_Key, char *Array, int LenArray);
String cmd1_encryption(char *AES_Key, char *Array, int LenArray);
/*CMD 2*/
bool cmd2_handle(cmd2_str *data);
String report_UpdateOTA(bool complete);
/*CMD 4*/
void UpdateConfig();
/*CMD 8*/
void sendmqtt_distance(uint32_t BaseID, uint32_t distance);
/*CMD 10*/
void sendmqtt_distance_cmd10(uint32_t BaseID, uint32_t distance);
/*CMD 11*/
void repost_accept(uint8_t accept);
#endif //_MQTT_DATA_BASE_H
