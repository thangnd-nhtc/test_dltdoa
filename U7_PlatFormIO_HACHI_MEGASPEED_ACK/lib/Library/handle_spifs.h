#ifndef __HANDLE_SPIFS_H
#define __HANDLE_SPIFS_H

#include <FS.h>
#include "Arduino.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <TimeOutEvent.h>
#include "handle_com_regs.h"
#include "define.h"

#define FILE_PARAMETTER_DW "/dw.txt"//"/config_decawave.json"
#define FWOTA    "/FWOTA.bin"
extern bool FlagOTA;
extern bool FlagWriteOTA;
extern TimeOutEvent checktimeOTA;
#define NAME_FILE_SIZE 100

class parametter
{
public:
	parametter(/* args */);
	~parametter();

	volatile serial_id_t serial_id;
	volatile master_t master;
	volatile master_access_t master_access[DECAWAVE_MASTER_ACCESS_NUM];
	volatile dw_config_t dw_config;
	volatile dw_txconfig_t dw_txconfig;
	volatile anten_delay_t anten_delay;
	bool SPIFFSbegin();
	bool checkfile(char* file_name);
	String readSerialprintln(char* namefile);
	bool read(char *file_name, uint16_t address, uint8_t *data);
	void save(char *file_name, uint16_t address, uint8_t *data);
	void del(char *file_name);
	bool createNewFileOTA(char *file_name);	
	uint8_t writeFile(char *path, uint8_t *buf, size_t size);
	uint8_t writeFileOTA(uint8_t *buf, size_t size);
	uint8_t checkMD5_FileOTA(char* md5);
	void reciverOTA(uint8_t *data, size_t length);
	bool calculator_md5(char *name, char *md5);
	char *sd_fix_file_name(char *file_name);
	uint8_t startUpdateFW(const char * path);
	uint8_t performUpdateFW(Stream &updateSource, size_t updateSize);

private:
	
	/** format chung để lưu và đọc dữ liệu ở file config
	 * Mã nhận dạng: mã để xác định đầu dữ liệu của 1 gói
	 * địa chỉ lưu: thể hiện gói dữ liệu này của loại nào
	 * dữ liệu: vùng để dữ liệu
	 * Mã nhận dạng | địa chỉ lưu | dữ liệu
	 * 4 byte|4 byte|   2 byte	  | fix lenght "COMMUNICATION_LENGHT_BUFFER"
	 */
	
	struct
	{
		uint32_t indentifi_1;
		uint32_t indentifi_2;
		uint16_t address;
		uint8_t data[COMMUNICATION_LENGHT_BUFFER];
	} frame_save;

#define FRAME_LENGTH sizeof(frame_save)
#define INDENTIFI_1 0x12345678
#define INDENTIFI_2 0X87654321
};

extern parametter parametter_dw;

#endif
