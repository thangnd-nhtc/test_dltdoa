/**
 *-SD_CARD-		-PIN FUNCTION-
 * 1 DATA2			X
 * 2 DATA3			CS
 * 3 CMD			MOSI
 * 4 VDD			3.3V
 * 5 CLK			SCLK
 * 6 VSS			GND
 * 7 DATA0			MISO
 * 8 DATA1			X
 * */

#include "handle_sdcard.h"

volatile bool sd_is_busy_f = false;
volatile bool sd_is_config_f = false;
volatile bool sd_is_full_memory_f = false;

TimeOutEvent sd_is_busy_to(0);

void sd_handle_config(void)
{
	// pinMode(SD_CARD_POWER, OUTPUT);
  	// digitalWrite(SD_CARD_POWER, LOW);

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;

	sd_is_busy_f = true;
	dbg_sd("SD Card is config..");

	uint8_t ret;
    ret = SPIFFS.begin(true);
    if(ret)
	{
		debug_SPIFFS("begin SPIFFS OK");
		sd_is_config_f = true;
		/*Khởi tạo file để làm mốc so sánh*/
		sd_is_busy_f = false;
		if (sd_check_file(SD_FILE_DEFAULT) == false)
			sd_create_file(SD_FILE_DEFAULT);
	}
    else
	{
		debug_SPIFFS("An Error has occurred while mounting SPIFFS");
		sd_is_config_f = false;
	}

	// digitalWrite(SPI_COM_CS, HIGH);
	// H_SPI.begin(14,12,13);  
	// if (SD.begin(SD_CARD_CS, H_SPI))
	// {
	// 	dbg_sd("SD Card is ready");
	// 	sd_is_config_f = true;

	// 	/*Khởi tạo file để làm mốc so sánh*/
	// 	sd_is_busy_f = false;
	// 	if (sd_check_file(SD_FILE_DEFAULT) == false)
	// 		sd_create_file(SD_FILE_DEFAULT);
	// }
	// else
	// {
	// 	dbg_sd("SD Card not config");
	// 	sd_is_config_f = false;
	// }
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
}

#define SD_INTERVAL_CHECK 10 * 60 * 1000
TimeOutEvent SD_TO_CHECK(SD_INTERVAL_CHECK);
void sd_check_reconfig(void)
{
	if (SD_TO_CHECK.ToEExpired() == false)
		return;

	/*Nếu đang bận thì sau 1 khoản thời gian check lại*/
	if (sd_is_busy() == true)
	{
		SD_TO_CHECK.ToEUpdate(30000);
		return;
	}
	SD_TO_CHECK.ToEUpdate(SD_INTERVAL_CHECK);

	/*Reconfig SD card*/
	dbg_sd("recheck sd card");
	if (sd_is_config_f == false || sd_check_file(SD_FILE_DEFAULT) == false)
	{
		SD_TO_CHECK.ToEUpdate(SD_INTERVAL_CHECK / 5);
		SPIFFS.end();
		sd_handle_config();
	}

	/*check SD card full memory*/
	if (sd_is_config_f == true)
	{
		size_t size_total = sd_size_total();
		size_t size_user = sd_size_user();
		if ((float)(size_total / size_user) < 1.2)
			sd_is_full_memory_f = true;
		else
			sd_is_full_memory_f = false;
	}
}

bool sd_is_ready(void)
{
	return sd_is_config_f;
}

bool sd_confirm_ok(void)
{
	/*Nếu chưa config được thì báo luôn*/
	if (sd_is_config_f == false)
		return false;

	/*Check lại file cho chắc*/
	if (sd_check_file(SD_FILE_DEFAULT) == true)
		return true;

	/*Nếu không tìm thấy file để làm mốc check status*/
	SD_TO_CHECK.ToEUpdate(3000);
	sd_is_config_f = false;
	return false;
}

bool sd_is_busy(void)
{
	return sd_is_busy_f;
}

bool sd_is_full_memory(void)
{
	return sd_is_full_memory_f;
}

char *sd_fix_file_name(char *file_name)
{
	static char name_file[NAME_FILE_SIZE + 10];

	memset(name_file, 0, NAME_FILE_SIZE + 10);
	if (file_name[0] == '/')
		strcpy(name_file, file_name);
	else
		sprintf(name_file, "/%s", file_name);

	return name_file;
	// return file_name;
}

void sd_list_file(char *dirname, uint8_t levels)
{
	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return;

	char *re_rew_name = sd_fix_file_name(dirname);

	dbg_sd("Listing directory: %s", re_rew_name);

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;

	File root = SPIFFS.open(re_rew_name);
	if (!root)
	{
		dbg_sd("Failed to open directory");
		root.close();
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return;
	}
	if (!root.isDirectory())
	{
		dbg_sd("Not a directory");
		root.close();
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return;
	}

	File file = root.openNextFile();
	while (file)
	{
		if (file.isDirectory())
		{
			dbg_sd("DIR : %s", file.name());
			if (levels)
			{
				sd_list_file((char *)file.name(), levels - 1);
			}
		}
		else
		{
			dbg_sd("FILE: %s, size %d", file.name(), file.size());
		}
		file = root.openNextFile();
	}
	file.close();
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
}

bool sd_list_one_file(bool status, char *dirname, char *data_reply)
{
	static File root;
	static File file;
	char *re_rew_name;

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return false;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true)
		;
	sd_is_busy_f = true;

	switch (status)
	{
	case false:
		re_rew_name = sd_fix_file_name(dirname);

		file.close();
		root.close();

		root = SPIFFS.open(re_rew_name);
		if (!root || !root.isDirectory())
		{
			/*tắt cờ báo bận*/
			sd_is_busy_f = false;
			return false;
		}
	case true:
		file = root.openNextFile();
		if (file)
		{
			strncpy(data_reply, file.name(), NAME_FILE_SIZE);

			dbg_sd("FILE: %s, size %d", file.name(), file.size());
			sd_is_busy_to.ToEUpdate(5);
			/*tắt cờ báo bận*/
			sd_is_busy_f = false;
			return true;
		}
		else
		{
			file.close();
			root.close();
			/*tắt cờ báo bận*/
			sd_is_busy_f = false;
			return false;
		}
	}
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
}
bool sd_check_file(char *file_name)
{
	char *re_rew_name = sd_fix_file_name(file_name);
	// dbg_sd("check file %s",re_rew_name);

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return false;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;

	dbg_sd("check file %s", re_rew_name);
	if (!SPIFFS.exists(re_rew_name))
	{
		dbg_sd("not file %s", re_rew_name);
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return false;
	}
	dbg_sd("file %s is ready", re_rew_name);
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
	return true;
}

void sd_create_dir(char *path)
{
	char *re_rew_name = sd_fix_file_name(path);

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;

	dbg_sd("mkdir %s", re_rew_name);
	if (!SPIFFS.mkdir(re_rew_name))
		dbg_sd("mkdir failed %s", re_rew_name);
	else
		dbg_sd("mkdir %s OK", re_rew_name);
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
}

void sd_remove_dir(char *path)
{
	char *re_rew_name = sd_fix_file_name(path);

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;

	dbg_sd("rmdir %s", re_rew_name);
	if (!SPIFFS.rmdir(re_rew_name))
		dbg_sd("rmdir failed %s", re_rew_name);

	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
}

void sd_read_file(char *path)
{
	char *re_rew_name = sd_fix_file_name(path);

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;

	dbg_sd("read_file %s", re_rew_name);
	File file = SPIFFS.open(re_rew_name);
	if (!file)
	{
		dbg_sd("Failed to open file for reading %s", re_rew_name);
		file.close();
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return;
	}

	while (file.available())
	{
		Serial.write(file.read());
	}
	file.close();
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
}

bool sd_read_block_file(bool status, char *path, uint8_t *data, size_t size)
{
	static File file;
	char *re_rew_name;

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return false;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true)
		;
	sd_is_busy_f = true;
	switch (status)
	{
	case false:
		file.close();
		re_rew_name = sd_fix_file_name(path);
		file = SPIFFS.open(re_rew_name);
		if (!file)
		{
			/*tắt cờ báo bận*/
			sd_is_busy_f = false;
			return false;
		}
	case true:
		if (file.available())
		{
			file.read(data, size);
			sd_is_busy_to.ToEUpdate(5);
			/*tắt cờ báo bận*/
			sd_is_busy_f = false;
			return true;
		}
		else
		{
			file.close();
			/*tắt cờ báo bận*/
			sd_is_busy_f = false;
			return false;
		}
	}
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
}

bool sd_read_block_data(char *path, uint8_t *data, size_t size)
{
	File file;

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return false;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true)
		;
	sd_is_busy_f = true;

	dbg_sd("read_block_data %s", path);
	file = SPIFFS.open(sd_fix_file_name(path));
	if (!file)
	{
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return false;
	}

	file.read(data, size);
	file.close();
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
	return true;
}

bool sd_create_file(char *path)
{
	char *re_rew_name = sd_fix_file_name(path);
	// char *re_rew_name = path;
	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return false;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;

	dbg_sd("create file %s", re_rew_name);

	File file = SPIFFS.open(re_rew_name, FILE_APPEND);
	if (!file)
	{
		dbg_sd("Failed to create file %s", re_rew_name);
		file.close();
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return false;
	}
	file.flush();
	file.close();
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
	return true;
}

void sd_write_file(char *path, char *message)
{
	char *re_rew_name = sd_fix_file_name(path);

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;

	dbg_sd("write_file %s", re_rew_name);
	File file = SPIFFS.open(re_rew_name, FILE_WRITE);
	if (!file)
	{
		dbg_sd("Failed to open file for writing %s", re_rew_name);
		file.close();
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return;
	}
	if (message != NULL)
		file.print(message);
	file.flush();
	file.close();
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
}

void sd_append_file(char *path, uint8_t *message, size_t size)
{
	char *re_rew_name = sd_fix_file_name(path);

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
	{
		dbg_sd("Failed");
		return;
	}
		

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;
	
	File file = SPIFFS.open(re_rew_name, FILE_APPEND);
	if (!file)
	{
		dbg_sd("Failed to open file for appending %s", re_rew_name);
		file.close();
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return;
	}

	file.setTimeout(5000);
	uint8_t retry = 0;
	while (retry < 10)
	{
		if(file.write(message, size) == size)
		{
			dbg_sd("write ok");
			sd_is_busy_f = false;
			file.close();
			return;
		}
		delay(1);
		retry++;
	}
	dbg_sd("write fail");
	file.flush();
	file.close();
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
}

static File fileOTA;
bool createNewFileOTA(char *file_name)
{
	fileOTA.close();
	if(sd_is_busy_f && !SPIFFS.begin())
	return false;

	if (SPIFFS.exists(file_name) == true)
		SPIFFS.remove(file_name);

	fileOTA = SPIFFS.open(file_name, FILE_APPEND);
	if (!fileOTA)
	{
		dbg_sd("Failed to create file %s", file_name);
		fileOTA.close();
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return false;
	}
	sd_is_busy_f = false;
	return true;	
}

uint8_t writeFileOTA(uint8_t *buf, size_t size)
{
	if(sd_is_busy_f && !SPIFFS.begin())
	return 0;

	if(!fileOTA.write(buf, size))
	{
		 dbg_sd("- write OTA failed");
		 sd_is_busy_f = false;
		 return 0;
	}
	else
		 dbg_sd("- write OTA ok");
		 sd_is_busy_f = false;
		 return 1;
}

void endWriteOTA()
{
	fileOTA.flush();
	fileOTA.close();
}

void sd_rename_file(char *path1, char *path2)
{
	char re_rew_name1[128];
	char re_rew_name2[128];
	strcpy(re_rew_name1, sd_fix_file_name(path1));
	strcpy(re_rew_name2, sd_fix_file_name(path2));

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;

	dbg_sd("rename_file %s-%s", re_rew_name1, re_rew_name2);
	if (SPIFFS.rename(re_rew_name1, re_rew_name2))
		dbg_sd("File renamed form %s to %s finish", re_rew_name1, re_rew_name2);
	else
		dbg_sd("File renamed form %s to %s fail", re_rew_name1, re_rew_name2);
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
}

bool sd_delete_file(char *path)
{
	char *re_rew_name = sd_fix_file_name(path);

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return false;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;

	dbg_sd("delete_file %s", re_rew_name);
	if (SPIFFS.remove(re_rew_name))
	{
		dbg_sd("File deleted finish %s", re_rew_name);
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return true;
	}
	else
	{
		dbg_sd("Delete failed");
		/*tắt cờ báo bận*/
		sd_is_busy_f = false;
		return false;
	}
	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
	return true;
}
size_t sd_size_file(char *file_name)
{
	File root;
	char *re_rew_name;

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return 0;

	/*bật cờ báo bận*/
	while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		;
	sd_is_busy_f = true;

	re_rew_name = sd_fix_file_name(file_name);

	root = SPIFFS.open(re_rew_name);
	size_t size_file = root.size();
	root.close();
	dbg_sd("size_file %s - %lu", re_rew_name,size_file);

	/*tắt cờ báo bận*/
	sd_is_busy_f = false;
	return size_file;
}

size_t sd_size_total(void)
{
	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return 0;

	return SPIFFS.totalBytes();
}

size_t sd_size_user(void)
{
	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (sd_is_config_f == false && !SPIFFS.begin(true))
		return 0;

	return SPIFFS.usedBytes();
}

#include <integer.h>
#include <ff.h>

void sd_format(void)
{
	// if (sd_is_config_f == false)
	// 	return;
	// /*bật cờ báo bận*/
	// while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
	// 	;
	// sd_is_busy_f = true;

	// void *workbuf = malloc(4096);
	// DWORD plist[] = {100, 0, 0, 0};
	// if (f_fdisk(0, plist, workbuf) == FR_OK)
	// 	f_mkfs("/SDHC", FM_FAT32, 16 * 1024, workbuf, 4096);
	// free(workbuf);
	// /*tắt cờ báo bận*/
	// sd_is_busy_f = false;
}
