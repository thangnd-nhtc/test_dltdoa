/*
Ktdtuan handle lib Log_File
Create 31/01/2020
Format File:
file_name	: logfile_yyyy_mm_dd.txt
data		: hh:mm:ss data
Handle Program:
1. Call log_file_loop. Process write log when real time ok and file ok
2. Call log_file_printf. write to the buffer data as printf function
*/

#ifndef _LOG_FILE_H
#define _LOG_FILE_H

// #define Logfile_debug(fmt, ...) //Serial.printfln(PSTR(">LogFile< " fmt), ##__VA_ARGS__)

// #define LOG_ENABLE 1
// #define LOG_DERECTORY "LOG"

void log_file_printf(const char *fmt, ...);

// void log_file_loop(void);

#endif //_LOG_FILE_H

/*THE END*/
