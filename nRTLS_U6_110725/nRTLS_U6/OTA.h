
#ifndef OTA_H
#define OTA_H

#include <FtpClientUpdate.h>
// #include "Log_File.h"
// #include "FileConfig.h"
// #include "Handle_FTP.h"
#include "handle_sdcard.h"
#include "md5.h"
#include <SD.h>
#include <Update.h>

#define Md5_Lenght 100
#define Path_Lenght 100
#define Fname_Lenght 100
#define Ver_Lenght 10

#include <Arduino.h>
extern bool FlagOTA;
extern bool FlagDownloand;
extern String g_u7_ota_status;
// #define debug_OTA(fmt, ...) //log_file_printf(PSTR(">OTA< " fmt), ##__VA_ARGS__)

// extern TimeOutEvent ESPRebootTo;

typedef enum
{
	ota_null,
	ota_loadfile,
	ota_check_load,
	ota_send_init,
	ota_send_size,
	ota_send_data,
	ota_read_data,
	ota_exit_data,
	ota_send_md5,
	ota_wait_reply,
	ota_reset_board,
	ota_update_ok,
} otaled_flag_t;
extern otaled_flag_t otaled_flag;

typedef struct
{
	bool Main_update;
	bool DW_update;
	char Md5[Md5_Lenght + 1];
	char Path[Path_Lenght + 1];
	char Fname[Fname_Lenght + 1];
} Update_TypeDef;
extern Update_TypeDef Update_info;
void OTSD_Main_Update(void);
void OTA_Main_update(char *FName, char *Path, char *MD5);
void check_OTA_DW();

void OTA_Main_loop(void);
void OTA_DW_loop();

void OTSD_LED_Update(void);
bool OTA_check_busy(void);
void OTA_DW_update(char *FName, char *Path, char *MD5);
void OTA_Led_reply(otaled_flag_t otaled_flag, char *cmd);
void OTA_Led_loop();

#endif //OTA_H
