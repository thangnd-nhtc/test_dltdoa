#ifndef __SD_Process_H
#define __SD_Process_H

#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "FS.h"
#include <SD.h>
#include "SPI.h"
#include "define.h"
#include "TimeOutEvent.h"

// #define sdcard_debug(fmt, ...) //Serial.printfln(PSTR(">SD Card< " fmt), ##__VA_ARGS__)

#define SD_FILE_DEFAULT "StatusSD.txt"
#define SD_FILE_CONFIG "baseconfig.txt"

#define DEFINE_CARD_CS_PIN 33

#define NAME_FILE_SIZE 100

void sd_handle_config(void);
bool sd_is_busy(void);
bool sd_is_ready(void);
bool sd_confirm_ok(void);
bool sd_is_full_memory(void);
void sd_check_reconfig(void);
char *sd_fix_file_name(char *file_name);
void sd_list_file(char *dirname, uint8_t levels);
bool sd_list_one_file(bool status, char *dirname, char *data_reply);
bool sd_check_file(char *file_name);
void sd_create_dir(char *path);
void sd_remove_dir(char *path);
void sd_read_file(char *path);
bool sd_read_block_file(bool status, char *path, uint8_t *data, size_t size);
bool sd_read_block_data(char *path, uint8_t *data, size_t size);
bool sd_create_file(char *path);
void sd_write_file(char *path, char *message);
void sd_append_file(char *path, uint8_t *message, size_t size);
void sd_rename_file(char *path1, char *path2);
bool sd_delete_file(char *path);
size_t sd_size_file(char *file_name);
size_t sd_size_total(void);
size_t sd_size_user(void);
void sd_format(void);

bool createNewFileOTA(char *file_name);
uint8_t writeFileOTA(uint8_t *buf, size_t size);
void endWriteOTA();
#endif //__SD_Process_H

/*THE END*/
