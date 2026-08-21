#include "Handle_FTP.h"
#include "define.h"
#include "DataBase.h"

FTPHandle::FTPHandle(/* args */) {}
FTPHandle::~FTPHandle() {}

void loop_sync(void *arg)
{
	bool status = false;
	debug_ftp("continue check_del_in_SD");
	FTP_Handle.check_del_in_SD();

	while (true)
	{
		debug_ftp("continue check_missing_in_SD");
		if ((status = FTP_Handle.check_missing_in_SD()) == false)
			break;
		debug_ftp("continue download_file_to_SD");
		if ((status = FTP_Handle.download_file_to_SD()) == false)
			break;
	}

	FTP_Handle.callBack(true);

	FTP_Handle.the_end();
}
void loop_download(void *arg)
{
	FTP_Handle.callBack(FTP_Handle.download_file_to_SD());

	FTP_Handle.the_end();
}

void FTPHandle::begin(char *server, char *user, char *pass)
{
	this->server = server;
	this->user = user;
	this->pass = pass;
}

bool FTPHandle::open(void)
{
	ftp = new ESP32_FTPClient(this->server, this->user, this->pass, 10000);

	/*Nếu connect tới FTP thất bại thì thoát*/
	if (ftp->OpenConnection() == false)
	{
		debug_ftp("open  %s, %s, %s Error", this->server, this->user, this->pass);
		this->_isLoading = false;
		return false;
	}

	/*Cấu hình FTP*/
	ftp->ChangeWorkDir(_PathofFolder);
	ftp->InitFile("Type I");

	debug_ftp("open %s, %s, %s OK", this->server, this->user, this->pass);

	return true;
}

bool FTPHandle::close(void)
{
	debug_ftp("Close OK");
	ftp->CloseConnection();
	delete ftp;
}

void FTPHandle::check_del_in_SD(void)
{
	char file_link[128];
	bool start = false;
	char *point_name;

	debug_ftp("Check delete file in SD card");

	while (start = sd_list_one_file(start, _PathofFolder, file_link) == true)
	{
		//strlwr(file_link);
		/*Nếu gặp file đang tải thì tự xóa*/
		if (strstr(file_link, "loading") != NULL)
		{
			debug_ftp("delete file %s", file_link);
			sd_delete_file(file_link);
			continue;
		}

		/*Chỉ lấy đúng tên file*/
		if ((point_name = strchr(&file_link[1], '/')) == 0)
			continue;
		strcpy(_FileofLoad, point_name + 1);

		if (this->open() == false)
			continue;
		/*nếu không có file này trên FTP thì sẽ xóa dưới SD*/
		if (ftp->CheckFileInList(_FileofLoad) == false)
		{
			debug_ftp("Check not file %s in FTP. Delete file: %s", _FileofLoad, file_link);
			sd_delete_file(file_link);
		}
		this->close();
		delay(1);
	}
}

bool FTPHandle::check_missing_in_SD(void)
{
	bool start_rd = 0;
	char file_link[FTP_NAME_SIZE];

	debug_ftp("Check folder %s in FTP and SD", _PathofFolder);

	memset(_FileofLoad, '\x00', FTP_NAME_SIZE);

	if (this->open() == false)
		return false;

	while ((start_rd = ftp->GetFNameInList(start_rd, _FileofLoad)) == true)
	{
		debug_ftp("Get file FTP %s", _FileofLoad);

		/*Nếu đã có file ở SD thì tiếp tục kiểm tra*/
		sprintf(file_link, "/%s/%s", _PathofFolder, _FileofLoad);
		debug_ftp("check file in SD %s", file_link);
		if (sd_check_file(file_link))
		{
			memset(_FileofLoad, '\x00', FTP_NAME_SIZE);
			continue;
		}

		debug_ftp("SD Card not file %s", _FileofLoad);

		this->close();
		return true;
	}
	debug_ftp("Successful sync");
	this->close();
	return false;
}

bool FTPHandle::download_file_to_SD(void)
{
	char file_link[FTP_NAME_SIZE];
	char file_link_down[FTP_NAME_SIZE + 20];
	size_t DataSize = 1 * 1024;
	size_t SizeRead = 0;
	size_t FileSize = 0;

	debug_ftp("Create file %s", _FileofLoad);

	/*create file in sd card*/
	memset(file_link, 0, FTP_NAME_SIZE);
	sprintf(file_link, "/%s/%s", _PathofFolder, _FileofLoad);
	//sprintf(file_link, "%s/%s", _PathofFolder, _FileofLoad); //Hieu
	sprintf(file_link_down, "/%s/%sloading", _PathofFolder, _FileofLoad);
	if (sd_create_file(file_link_down) == false)
		return false;

	/*lấy kích thước file*/
	this->open();
	FileSize = ftp->GetFileSize(_FileofLoad);
	debug_ftp("File %s size %d", _FileofLoad, FileSize);
	this->close();

	/*định dạng lại block tải file nếu kích thước file nhỏ hơn block mặt định*/
	if (FileSize < DataSize)
		DataSize = FileSize;

	uint8_t *DataBuff = (uint8_t *)malloc(DataSize + 10);

	this->open();
	bool start = 0;
	while ((start = ftp->ReadDataInFile(start, _FileofLoad, FileSize, DataBuff, DataSize)) == true)
	{
		sd_append_file(file_link_down, DataBuff, DataSize);
		memset(DataBuff, 0, sizeof(DataBuff));
		delay(1);

		debug_ftp("%d Load. %d/%d", DataSize, SizeRead, FileSize);

		/*tính kích thước còn lại*/
		SizeRead += DataSize;
		if (FileSize - SizeRead < DataSize)
			DataSize = FileSize - SizeRead;
	}

	this->close();
	free(DataBuff);

	/*Nếu check MD5 thất bại thì xóa file, vòng lập sau làm lại*/
	if (this->check_md5(file_link_down, NULL) == false)
	{
		debug_ftp("MD5 fail");
		sd_delete_file(file_link_down);
		return false;
	}
	/*Tải thành công*/
	debug_ftp("Download file %s finish", _FileofLoad);
	sd_rename_file(file_link_down, file_link);
	return true;
}

bool FTPHandle::check_md5(char *name, char *md5)
{
	String MD5 = "";
	/*nếu không có cần checksum*/
	if (md5 != NULL)
	{
		MD5 = String(md5);
	}
	// else
	// {
	// 	String f_name = String(name);
	// 	size_t index = f_name.indexOf(".mp3");
	// 	if (index >= 32)
	// 	{
	// 		MD5 = f_name.substring(index - 32, index);
	// 		MD5.toUpperCase();
	// 	}
	// 	else
	// 		return true;
	// }

	/*đọc file ra check md5 trong sd card*/
	bool start = 0;
	size_t size_block = 1024;
	uint8_t *buff_data = (uint8_t *)malloc(size_block + 10);
	size_t file_size = sd_size_file(name);

	MD5_CTX md5_check;
	MD5Init(&md5_check);

	while ((start = sd_read_block_file(start, name, buff_data, size_block)) == true)
	{
		delay(1);
		
		size_t current_chunk = (file_size >= size_block) ? size_block : file_size;
		MD5Update(&md5_check, buff_data, current_chunk);
		
		if (file_size <= size_block)
			break;
			
		file_size -= size_block;
	}
	free(buff_data);
	/*Tổng hợp check md5*/
	uint8_t md5_hash[16 + 1];
	uint8_t md5_hash_buff[32 + 1];

	MD5Final(md5_hash, &md5_check);
	for (uint8_t i = 0; i < 16; i += 1)
		sprintf((char *)&md5_hash_buff[i * 2], "%02X", md5_hash[i]);

	debug_ftp("md5 %s-%s", md5_hash_buff, MD5.c_str());
	/*Nếu compare MD5md5 sai thì xóa file*/
	if (memcmp(md5_hash_buff, MD5.c_str(), 32))
	{
		debug_ftp("File %s is md5 fail", name);
		return false;
	}
	else
		return true;
}

bool FTPHandle::calculator_md5(char *name, char *md5)
{
	/*đọc file ra check md5 trong sd card*/
	bool start = 0;
	size_t size_block = 1024;
	uint8_t *buff_data = (uint8_t *)malloc(size_block + 10);
	size_t file_size = sd_size_file(name);
	if(!file_size)
	{
		debug_ftp("calculator md5 fail");
		return false;
	}
		

	MD5_CTX md5_check;
	MD5Init(&md5_check);

	while ((start = sd_read_block_file(start, name, buff_data, size_block)) == true)
	{
		delay(1);
		
		size_t current_chunk = (file_size >= size_block) ? size_block : file_size;
		MD5Update(&md5_check, buff_data, current_chunk);
		
		if (file_size <= size_block)
			break;
			
		file_size -= size_block;
	}
	free(buff_data);
	/*Tổng hợp check md5*/
	uint8_t md5_hash[16 + 1];
	uint8_t md5_hash_buff[32 + 1];

	MD5Final(md5_hash, &md5_check);
	for (uint8_t i = 0; i < 16; i += 1)
		sprintf((char *)&md5_hash_buff[i * 2], "%02X", md5_hash[i]);
	
	memcpy(md5, md5_hash_buff, 32);
	return true;
}

bool FTPHandle::sync(char *path, HandlerFunction handler)
{
	debug_ftp("sync");
	if (this->_isLoading == true || sd_is_ready() == false)
		return false;

	strcpy(_PathofFolder, path);
	debug_ftp("sync %s", _PathofFolder);

	/*void callBack*/
	this->_callBack = handler;

	/*và thư mục lưu trữ, không có thì tạo.*/
	if (sd_check_file(_PathofFolder) == false)
		sd_create_dir(_PathofFolder);

	xTaskCreatePinnedToCore(loop_sync, "ftp_sync", 4096, NULL, 2, &task_ftp_load, 1);
	this->_isLoading = true;
	return true;
}

bool FTPHandle::download(char *file, HandlerFunction handler)
{

	debug_ftp("download");
	if (this->_isLoading == true || sd_is_ready() == false)
		return false;

	strcpy(_PathofFolder, "upload");
	strcpy(_FileofLoad, file);

	/*void callBack*/
	this->_callBack = handler;

	/*và thư mục lưu trữ, không có thì tạo.*/
	if (sd_check_file(_PathofFolder) == false)
		sd_create_dir(_PathofFolder);

	/*Kiểm tra đã có file đó chưa*/
	String link_file = String(_PathofFolder) + "/" + String(_FileofLoad);
	debug_ftp("download %s", link_file.c_str());
	if (sd_check_file((char *)link_file.c_str()))
		return true;

	xTaskCreatePinnedToCore(loop_download, "ftp_download", 4096, NULL, 2, &task_ftp_load, 1);
	this->_isLoading = true;
	return true;
}

bool FTPHandle::download(char *path, char *file, char *md5)
{
	// debug_ftp("download");
	if (this->_isLoading == true || sd_is_ready() == false)
		return false;

	/*Bật cờ báo bận*/
	this->_isLoading = true;
	strcpy(_PathofFolder, path);

	/*và thư mục lưu trữ, không có thì tạo.*/
	// if (sd_check_file(path) == false)
	// 	sd_create_dir(path);

	/*Kiểm tra đã có file đó chưa*/
	// String link_file = String(path) + "/" + String(file);
	String link_file = String((char*)file);
	debug_ftp("download %s", link_file.c_str());
	if (sd_check_file((char *)link_file.c_str()))
		sd_delete_file((char *)link_file.c_str());

	/*download file*/
	String link_file_loading;
	size_t DataSize = 100 * 1024;
	size_t SizeRead = 0;
	size_t FileSize = 0;

	/*create file in sd card*/
	link_file_loading = link_file ; //+ "loading";
	if (sd_create_file((char *)link_file_loading.c_str()) == false)
	{
		this->_isLoading = false;
		return false;
	}
	if(sd_check_file((char *)link_file_loading.c_str()))
	debug_ftp("Create file complete");
	else
	{
		debug_ftp("Create file fail");
		return false; 
	}
	/*lấy kích thước file*/
	this->open();
	FileSize = ftp->GetFileSize(file);
	debug_ftp("File %s size %d", file, FileSize);
	this->close();

	/*định dạng lại block tải file nếu kích thước file nhỏ hơn block mặt định*/
	if (FileSize < DataSize)
		DataSize = FileSize;

	uint8_t *DataBuff = (uint8_t *)ps_malloc(DataSize + 10);
	this->open();
	bool start = 0;
	memset(DataBuff, 0, sizeof(DataBuff));
	// createNewFileOTA((char *)link_file_loading.c_str());
	while ((start = ftp->ReadDataInFile(start, file, FileSize, DataBuff, DataSize)) == true)
	{
		// debug_ftp("read data file");
		sd_append_file((char *)link_file_loading.c_str(), DataBuff, DataSize);
		// if(writeFileOTA( DataBuff, DataSize))
		// 	debug_ftp("ok");
		memset(DataBuff, 0, sizeof(DataBuff));
		
		/*tính kích thước còn lại*/
		SizeRead += DataSize;
		if (FileSize - SizeRead < DataSize)
			DataSize = FileSize - SizeRead;
	    debug_ftp("%d Load. %d/%d", DataSize, SizeRead, FileSize);
		delay(1);
	}
	// endWriteOTA();
	this->close();
	free(DataBuff);

	/*Nếu check MD5 thất bại thì xóa file, vòng lập sau làm lại*/
	if (this->check_md5((char *)link_file_loading.c_str(), md5) == false)
	{
		debug_ftp("MD5 fail");
		sd_delete_file((char *)link_file_loading.c_str());
		this->_isLoading = false;
		return false;
	}
	
	/*Tải thành công*/
	debug_ftp("Download file %s finish", file);
	if(sd_check_file((char*)OTSD_Firmware_LED))
		sd_delete_file((char*)OTSD_Firmware_LED);
		
	if(sd_check_file((char *)link_file_loading.c_str()))
		sd_rename_file((char *)link_file_loading.c_str(),(char*)OTSD_Firmware_LED);  //(char *)link_file.c_str()
	this->_isLoading = false;
	return true;
}

void FTPHandle::callBack(bool status)
{
	this->_callBack(status);
}

void FTPHandle::the_end(void)
{
	this->_isLoading = false;

	vTaskDelete(task_ftp_load);
}
FTPHandle FTP_Handle;
