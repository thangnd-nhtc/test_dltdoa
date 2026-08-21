
#include <WiFi.h>

//#include "handle_logfile.h"
// #include "handle_sdcard.h"

// /******************************Create and write dat to file**********************************/

// char *LOG_FILE_HEADER = "LOG_";

// void lf_check_memory(void)
// {
// 	char file_delete[50];
// 	uint8_t min_day = 0xFF, min_month = 0xFF, min_year = 0xFF;
// 	signed int now_day = 0xFF, now_month = 0xFF, now_year = 0xFF;

// 	size_t size_total = sd_size_total();
// 	size_t size_user = sd_size_user();
// 	if (size_total == 0 || size_user == 0)
// 		return;

// 	if ((float)(size_total / size_user) < 1.5)
// 	{
// 		bool start = false;
// 		while ((start = sd_list_one_file(start, LOG_DERECTORY, file_delete)) == true)
// 		{
// 			delay(1);
// 			/*không phải dịnh dạng file LOG bỏ qua*/
// 			char *point = strstr(file_delete, LOG_FILE_HEADER);
// 			if (point == NULL)
// 				continue;
// 			// Logfile_debug("detect file: %s", point);
// 			/*không đúng fomat thời gian file LOG bỏ qua*/
// 			if (sscanf(point + 4, "%02d%02d%02d.txt", &now_year, &now_month, &now_day) != 3)
// 				continue;

// 			// Logfile_debug("detect: %02d-%02d-%02d", now_year, now_month, now_day);

// 			/*Thời gian của file hiện tại, lớn hơn thời gian của file cần xóa => bỏ qua*/
// 			if (now_year > min_year)
// 				continue;
// 			min_year = now_year;
// 			if (now_month > min_year)
// 				continue;
// 			min_month = now_month;
// 			if (now_day > min_day)
// 				continue;
// 			min_day = now_day;

// 			// Logfile_debug("confirm: %02d-%02d-%02d", min_year, now_month, now_day);
// 		}

// 		/*Nếu không tìm được file cần xóa*/
// 		if (min_day == 0xFF || min_month == 0xFF || min_year == 0xFF)
// 			return;

// 		/*Xóa file có time thời gian tạo cũ nhất*/
// 		sprintf(file_delete, "%s/%s%02d%02d%02d.txt", LOG_DERECTORY, LOG_FILE_HEADER, min_year, min_month, min_day);
// 		sd_delete_file(file_delete);
// 		Logfile_debug("SD full disk, delete file: %s", file_delete);
// 	}
// }

// String format_file(struct tm *time)
// {
// 	char file_name[12];
// 	sprintf(file_name, "%02d%02d%02d.log", time->tm_year, time->tm_mon, time->tm_mday);
// 	String file = LOG_DERECTORY + "/" + file_name;

// 	return file;
// }

// bool lf_write_file(char *data)
// {
// 	struct tm rtc_time;

// 	if (LOG_ENABLE == 0)
// 		return false;

// 	/*kiểm tra dung lượng và xóa file*/
// 	lf_check_memory();

// 	/*Kiểm tra tạo thư mục LOG*/
// 	if (sd_check_file(LOG_DERECTORY) == false)
// 		sd_create_dir(LOG_DERECTORY);

// 	if (ESPTime_Handle.isOK() == false)
// 		return false;

// 	/*Lấy tên file theo fomat thời gian*/
// 	Time_Handle.Read_Time_Local(&rtc_time);

// 	char file_name[50];
// 	sprintf(file_name, "%s/%s%02d%02d%02d.txt",
// 			LOG_DERECTORY, LOG_FILE_HEADER,
// 			rtc_time.tm_year, rtc_time.tm_mon, rtc_time.tm_mday);

// 	while (sd_is_busy())
// 		;
// 	/*Kiểm tra và tạo file theo fomat thời gian*/
// 	if (sd_check_file(file_name) == false)
// 		sd_create_file(file_name);

// 	sd_append_file(file_name, (uint8_t *)(data), strlen(data));

// 	return true;
// }

// bool lf_read_file(const char *path)
// {
// 	File file_log;

// 	file_log = SD.open(path);
// 	if (!file_log || file_log.isDirectory())
// 	{
// 		Logfile_debug("failed to open file for reading");
// 		file_log.close();
// 		return false;
// 	}

// 	while (file_log.available())
// 	{
// 		Serial.write(file_log.read());
// 	}
// 	file_log.close();

// 	return true;
// }
// /*ghi nhan du lieu va ghi vao file*/
// #define LF_BuffShare_Len 1024
// char LF_BuffShare[1024];

// int16_t LF_BuffShare_IdWr;
// int16_t LF_BuffShare_IdRr;

// void lf_give_buff(const char *fmt, va_list argp)
// {
// 	char buffer[1024];

// 	if (0 < vsprintf(buffer, fmt, argp)) // build buffer
// 	{
// 		String log_data = Time_Handle.Read_Time_Local() + String(buffer) + "\r\n";
// 		memset(buffer, 0, sizeof(buffer));
// 		strncpy(buffer, (char *)log_data.c_str(), (log_data.length() > 1024) ? 1023 : log_data.length());
// 		for (uint16_t i = 0; i < strlen(buffer); i++)
// 		{
// 			LF_BuffShare[LF_BuffShare_IdWr++] = buffer[i];
// 			if (LF_BuffShare_IdWr >= LF_BuffShare_Len)
// 				LF_BuffShare_IdWr = 0;
// 		}
// 	}
// }
// void lf_take_buff(char *Buf, uint16_t Len)
// {
// 	uint16_t i;
// 	for (i = 0; i < Len; i++)
// 	{
// 		if (LF_BuffShare_IdWr == LF_BuffShare_IdRr)
// 			break;
// 		Buf[i] = LF_BuffShare[LF_BuffShare_IdRr++];
// 		if (LF_BuffShare_IdRr >= LF_BuffShare_Len)
// 			LF_BuffShare_IdRr = 0;
// 	}
// }
// int16_t lf_check_buff(void)
// {
// 	int16_t Num = LF_BuffShare_IdWr - LF_BuffShare_IdRr;
// 	Num += (Num < 0) ? LF_BuffShare_Len : 0;
// 	return Num;
// }
// void log_file_loop(void)
// {
// 	if (sd_is_busy() == true)
// 		return;
// 	if (FTP_Handle.isBusy() == true)
// 		return;
// 	if (ESPTime_Handle.isOK() == false)
// 		return;

// 	uint16_t Len = lf_check_buff();
// 	if (Len == 0)
// 		return;
// 	char buffer[Len + 1];

// 	lf_take_buff(buffer, Len);
// 	buffer[Len] = 0;

// 	lf_write_file(buffer);
// }
/*Tiếp nhận dữ liệu*/
void log_file_printf(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	// lf_give_buff(fmt, argp);
	va_end(argp);
}

/*THE END*/
