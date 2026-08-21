#ifndef __HANDLE_FTP_H
#define __HANDLE_FTP_H

#include "SPI.h"
#include "FS.h"

#include "ESP32_FTPClient.h"

#include <functional>

#include "md5.h"
#include "handle_sdcard.h"



#define FTP_NAME_SIZE 100

class FTPHandle
{
  protected:
	MD5_CTX md5;
	ESP32_FTPClient *ftp;

	TaskHandle_t task_ftp_load;

  public:
	FTPHandle(/* args */);
	~FTPHandle();
	typedef std::function<void(bool)> HandlerFunction;

	void begin(char *server, char *user, char *pass);

	bool isBusy(void)
	{
		return _isLoading;
	}

	bool sync(char *path, HandlerFunction handler);
	bool download(char *file, HandlerFunction handler);
	bool download(char *path, char *file, char *md5);

	//ở ngoài không gọi
	void check_del_in_SD(void);
	bool check_missing_in_SD(void);
	bool download_file_to_SD(void);
	bool calculator_md5(char *name, char *md5);
	void callBack(bool status);
	void the_end(void);

  private:
	bool _isLoading = false;

	char *server;
	char *user;
	char *pass;

	char _FileofLoad[FTP_NAME_SIZE];
	char _PathofFolder[FTP_NAME_SIZE];

	HandlerFunction _callBack;

	//ở ngoài không gọi
	bool open(void);
	bool close(void);

	bool check_md5(char *name, char *md5);
};

extern FTPHandle FTP_Handle;

#endif
