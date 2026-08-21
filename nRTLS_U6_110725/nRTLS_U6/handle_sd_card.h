
#ifndef __HANDLE_SDCARD_H
#define __HANDLE_SDCARD_H

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <TimeOutEvent.h>

#include "define.h"

#define STATUS_SD_FILE "nRTLS_CheckSD.txt"

class SDHandle
{
public:
	SDHandle(/* args */);
	~SDHandle();




	bool begin(void);

private:
	bool isBussy_f = false;
	bool isReady_f = false;
	TimeOutEvent *isBussy_TO;

	bool wait_bussy(void);
	void set_bussy(uint32_t timeout);
	void release_bussy(void);
};

extern SDHandle SD_Handle;
#endif //__HANDLE_SDCARD_H
