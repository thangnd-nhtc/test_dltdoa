#include "handle_spifs.h"
#include "md5.h"
#include "Update.h"
#include "handle_com.h"
#include "handle_spis.h"
#include "HardwareSerial.h"

/* CRC16 dùng chung polynomial 0x8408 (giống calcCRC trong handle_spi_master/handle_spis) */
static uint16_t ota_calcCRC16(const uint8_t *data, size_t size) {
	uint16_t CrcPoly = 0x8408;
	uint16_t crc = 0;
	for (size_t j = 0; j < size; j++) {
		crc ^= data[j];
		for (int i = 0; i < 8; i++) {
			uint8_t carry = crc & 1;
			crc >>= 1;
			if (carry) crc ^= CrcPoly;
		}
	}
	return crc;
}

bool Flagbusy = false;
bool FlagOTA = false;
static MD5_CTX md5_check;
TimeOutEvent checktimeOTA;
uint16_t size_nhan = 0;
static uint8_t buffer_nhan[1024 + 2 + 1]; /* +2 cho CRC16 từ U6 */
static uint16_t len_write = 0;
bool FlagWriteOTA = false;
/**
 * HÀM XỬ LÝ CÁC BIẾN DÀNH DEVICE
 * */
parametter::parametter(/* args */)
{
}
parametter::~parametter() {}

// static File fileOTA;

bool parametter::SPIFFSbegin()
{
	bool ret;
	ret = SPIFFS.begin(true);
	if (ret)
		dbg_spifs("begin SPIFFS OK");
	else
		dbg_spifs("An Error has occurred while mounting SPIFFS");
	return ret;
}

bool parametter::checkfile(char *file_name)
{
	if (Flagbusy && !this->SPIFFSbegin())
		return 0;

	if (SPIFFS.exists(file_name) == true)
	{
		Flagbusy = true;
		return 1;
	}
	Flagbusy = true;
	return 0;
}

// void parametter::save(char *file_name, uint16_t address, uint8_t *data)
// {
// 	if (Flagbusy && !this->SPIFFSbegin())
// 		return;

// 	Flagbusy = true;
// 	size_t num_block = 0;
// 	size_t size_file = 0;
// 	uint8_t *RawData;
// 	File file_pcs;
// 	/*Nếu đã có file*/
// 	if (SPIFFS.exists(file_name) == true)
// 	{
// 		// dbg_spifs("co file");
// 		file_pcs = SPIFFS.open(file_name);
// 		size_file = file_pcs.size();
// 		RawData = (uint8_t *)malloc(size_file + FRAME_LENGTH + 30);
// 		file_pcs.read(RawData, size_file);
// 		file_pcs.close();
// 		// dbg_spifs("file size:%d", size_file);
// 		//20240622
// 	 	//for (int i = 0; i < FRAME_LENGTH; i++)
// 	 	//	Serial.printf("%02X", RawData[++num_block]);
// 		delay(1);
// 		for (num_block = 0; num_block < size_file; num_block += FRAME_LENGTH)
// 		{
// 			// dbg_spifs("block:%d", num_block);
// 			memcpy((uint8_t *)&this->frame_save, &RawData[num_block], FRAME_LENGTH);
// 			// nếu đúng dữ liệu cần đọc thì thoát ra
// 			if (this->frame_save.indentifi_1 == INDENTIFI_1 &&
// 				this->frame_save.indentifi_2 == INDENTIFI_2 &&
// 				this->frame_save.address == address)
// 			{
// 				dbg_spifs("block index check %d = true", num_block);
// 				break;
// 			}

// 			// nếu khác dữ liệu nhận dạng thì xóa hết
// 			if (this->frame_save.indentifi_1 != INDENTIFI_1 &&
// 				this->frame_save.indentifi_2 != INDENTIFI_2)
// 			{
// 				dbg_spifs("format error, delete file config %d \n\r", num_block);
// 				// for (int i = 0; i < FRAME_LENGTH; i++)
// 				// 	Serial.printf("%x", RawData[++num_block]);

// 				// xóa file làm lại từ đầu
// 				// this->del(file_name);
// 				// gọi chính nó để lưu lại
// 				this->save(file_name, address, data);
// 				Flagbusy = false;
// 				free(RawData);
// 				return;
// 			}
// 		}
// 		// dbg_spifs("khong co truong dia chi do");
// 	}
// 	else
// 		RawData = (uint8_t *)malloc(FRAME_LENGTH + 10);

// 	if (num_block >= size_file)
// 		size_file += FRAME_LENGTH;

// 	//Đinh dạng lại frame để lưu vào file
// 	this->frame_save.indentifi_1 = INDENTIFI_1;
// 	this->frame_save.indentifi_2 = INDENTIFI_2;
// 	this->frame_save.address = address;
// 	memcpy(this->frame_save.data, data, COMMUNICATION_LENGHT_BUFFER);
// 	memcpy(&RawData[num_block], (uint8_t *)&this->frame_save, FRAME_LENGTH);

// 	// dbg_spifs("%s", (char*)RawData);
// 	// Ghi du lieu vao file
// 	// if (SPIFFS.exists(file_name) == true)
// 	// 	SPIFFS.remove(file_name);
	
// 	file_pcs = SPIFFS.open(file_name, "w");
// 	if (file_pcs.write(RawData, size_file))
// 		dbg_spifs("file %s, write %d byte", file_name, size_file);
// 	else
// 		dbg_spifs("write fail");
// 	file_pcs.close();

// 	// uint8_t datatest[500];
// 	// String a = this->readSerialprintln(file_name).c_str();
// 	// memcpy(datatest,a.c_str(),size_file);
// 	// for(int i = 0; i < size_file; i++)
// 	// Serial.printf("%x",datatest[i]);
// 	// giải phóng ram
// 	Flagbusy = false;
// 	free(RawData);
// }

// Chat GPT
void parametter::save(char *file_name, uint16_t address, uint8_t *data)
{
    if (Flagbusy && !this->SPIFFSbegin())
        return;

    Flagbusy = true;
    size_t num_block = 0;
    size_t size_file = 0;
    uint8_t *RawData = nullptr;
    File file_pcs;

    bool need_rewrite = false; // Cờ kiểm tra cần xóa và ghi lại file

    /* Nếu file đã tồn tại */
    if (SPIFFS.exists(file_name))
    {
        file_pcs = SPIFFS.open(file_name, "r"); // Mở file ở chế độ đọc
        size_file = file_pcs.size();
        RawData = (uint8_t *)malloc(size_file + FRAME_LENGTH + 30);
        if (!RawData)
        {
            dbg_spifs("malloc failed!");
            file_pcs.close();
            Flagbusy = false;
            return;
        }
        file_pcs.read(RawData, size_file);
        file_pcs.close();

        for (num_block = 0; num_block < size_file; num_block += FRAME_LENGTH)
        {
            memcpy((uint8_t *)&this->frame_save, &RawData[num_block], FRAME_LENGTH);

            if (this->frame_save.indentifi_1 == INDENTIFI_1 &&
                this->frame_save.indentifi_2 == INDENTIFI_2 &&
                this->frame_save.address == address)
            {
                dbg_spifs("block index check %d = true", num_block);
                break;
            }

            if (this->frame_save.indentifi_1 != INDENTIFI_1 ||
                this->frame_save.indentifi_2 != INDENTIFI_2)
            {
                dbg_spifs("format error, delete file %s", file_name);
                need_rewrite = true;
                break;
            }
        }
    }
    else
    {
        RawData = (uint8_t *)malloc(FRAME_LENGTH + 10);
        if (!RawData)
        {
            dbg_spifs("malloc failed!");
            Flagbusy = false;
            return;
        }
    }

    // Nếu cần xóa file do lỗi định dạng
    if (need_rewrite)
    {
        SPIFFS.remove(file_name);
        free(RawData);
        Flagbusy = false;
        return; // Không gọi lại `save()` để tránh đệ quy vô hạn
    }

    if (num_block >= size_file)
        size_file += FRAME_LENGTH;

    // Định dạng lại frame để lưu vào file
    this->frame_save.indentifi_1 = INDENTIFI_1;
    this->frame_save.indentifi_2 = INDENTIFI_2;
    this->frame_save.address = address;
    memcpy(this->frame_save.data, data, COMMUNICATION_LENGHT_BUFFER);
    memcpy(&RawData[num_block], (uint8_t *)&this->frame_save, FRAME_LENGTH);

    // Ghi dữ liệu vào file
    file_pcs = SPIFFS.open(file_name, "w");
    if (!file_pcs)
    {
        dbg_spifs("Failed to open file %s for writing", file_name);
    }
    else
    {
        if (file_pcs.write(RawData, size_file))
            dbg_spifs("file %s, write %d bytes", file_name, size_file);
        else
            dbg_spifs("write failed!");
        file_pcs.close();
    }

    // Giải phóng bộ nhớ
    free(RawData);
    Flagbusy = false;
}


String parametter::readSerialprintln(char *namefile) // đọc thông tin file config
{
	String data = "";
	File dataConfig = SPIFFS.open(namefile, FILE_READ);
	if (dataConfig.isDirectory())
	{
		dbg_spifs("Error, read config is not a file");
	}
	else
	{
		data = dataConfig.readString();
	}
	dataConfig.close();
	return data;
}

bool parametter::read(char *file_name, uint16_t address, uint8_t *data)
{
	if (Flagbusy && !this->SPIFFSbegin())
		return false;

	Flagbusy = true;
	if (SPIFFS.exists(file_name) == false)
	{
		Flagbusy = false;
		return false;
	}

	size_t num_block = 0;
	size_t size_file = 0;
	uint8_t *RawData;
	File file_pcs;
	file_pcs = SPIFFS.open(file_name);
	size_file = file_pcs.size();
	RawData = (uint8_t *)malloc(size_file + 10);
	file_pcs.read(RawData, size_file);
	file_pcs.close();

	dbg_spifs("file %s size %d", file_name, size_file);

	for (num_block = 0; num_block < size_file; num_block += FRAME_LENGTH)
	{
		memcpy((uint8_t *)&this->frame_save, &RawData[num_block], FRAME_LENGTH);
		if (this->frame_save.indentifi_1 == INDENTIFI_1 &&
			this->frame_save.indentifi_2 == INDENTIFI_2 &&
			this->frame_save.address == address)
		{
			dbg_spifs("block index check %d = true, add : %x", num_block, this->frame_save.address);
			memcpy(data, this->frame_save.data, COMMUNICATION_LENGHT_BUFFER);
			Flagbusy = false;
			return true;
		}
	}
	Flagbusy = false;
	return false;
}

void parametter::del(char *file_name)
{
	if (SPIFFS.exists(file_name) == true)
		SPIFFS.remove(file_name);
}

static File fileOTA;
bool parametter::createNewFileOTA(char *file_name)
{
	fileOTA.close();
	if (Flagbusy && !this->SPIFFSbegin())
		return false;

	if (SPIFFS.exists(file_name) == true)
		SPIFFS.remove(file_name);

	fileOTA = SPIFFS.open(file_name, FILE_APPEND);
	if (!fileOTA)
	{
		dbg_spifs("Failed to create file %s", file_name);
		fileOTA.close();
		/*tắt cờ báo bận*/
		Flagbusy = false;
		return false;
	}
	Flagbusy = false;
	// memset(&md5_check,0,sizeof(md5_check));
	MD5Init(&md5_check);
	return true;
}

uint8_t parametter::writeFileOTA(uint8_t *buf, size_t size)
{
	if (Flagbusy && !this->SPIFFSbegin())
		return 0;

	if (!fileOTA.write(buf, size))
	{
		dbg_spifs("- write OTA failed");
		Flagbusy = false;
		return 0;
	}
	else
		dbg_spifs("- write OTA ok");
	Flagbusy = false;
	return 1;
}

uint8_t parametter::checkMD5_FileOTA(char *md5)
{
	fileOTA.flush();
	fileOTA.close();

	uint8_t md5_hash[16 + 1];
	uint8_t md5_hash_buff[32 + 1];
	MD5Final(md5_hash, &md5_check);
	/*convert value array to data hex array*/
	for (uint8_t i = 0; i < 16; i += 1)
		sprintf((char *)&md5_hash_buff[i * 2], "%02X", md5_hash[i]);
	// check md5;
	for (int i = 0; i < 32; i++)
		dbg_spifs("%c", md5_hash_buff[i]);
	dbg_spifs("%s", "");
	if (memcmp((char *)md5_hash_buff, md5, 32))
	{
		dbg_spifs("MD5 fail");
		return 0;
	}
	else
	{
		dbg_spifs("MD5 complete");
		return 1;
	}
}

uint8_t parametter::writeFile(char *path, uint8_t *buf, size_t size)
{
	// Serial.printf("Writing file: %s\r\n", path);
	uint8_t ret = 1;
	File file = SPIFFS.open(path, FILE_WRITE);
	if (!file)
	{
		dbg_spifs("- failed to open file for writing");
		ret = 0;
	}
	else
	{
		for (int i = 0; i < size; i++)
		{
			if (file.write(buf, size))
			{
				dbg_spifs("- file written");
			}
			else
			{
				dbg_spifs("- write failed");
				ret = 0;
			}
		}
	}
	file.flush();
	file.close();

	return ret;
}

void parametter::reciverOTA(uint8_t *data, size_t length)
{
	// write_ota_t write_data;
	// memset(&write_data, 0, sizeof(&write_data));
	// memcpy((uint8_t *)&write_data, data, length);

	// memcpy(buffer,data,length);
	// dbg_spifs("write_OTA size %d", write_data.size_data);
	if (strncmp((char *)data, "SIZE_OTA:", 9) == 0 && !FlagWriteOTA)
	{
		uint8_t buffer[10];
		memset(buffer, 0, 10);
		strcpy((char *)buffer, (char *)(data + 9));
		size_nhan = atoi((char *)buffer);
		dbg_spifs("size nhan:%d\n\r", size_nhan);
		Serial1.flush();
		Serial1.print("OK\n");
		memset(buffer_nhan, 0, sizeof(buffer_nhan));
		len_write = 0;
		FlagWriteOTA = true;
	}

	while (FlagWriteOTA)
	{
		if (size_nhan == (uint16_t)0xFAFA)
		{
			if (Serial1.readBytes(buffer_nhan, 32))
			{
				dbg_spifs("write ota complete - MD5: ");
				for (int i = 0; i < 32; i++)
					dbg_spifs("%c", (char)buffer_nhan[i]);
				dbg_spifs("%s", "");

				frame_work_t frame_work;
				memset(&frame_work, 0, sizeof(frame_work));
				frame_work.header.type = infor_ota;
				frame_work.header.msk_regs.add = START_OTA.add;
				write_ota_t start_ota;
				if (this->checkMD5_FileOTA((char *)buffer_nhan))
				{
					if (this->startUpdateFW(FWOTA))
					{
						memset((uint8_t *)&start_ota, 0, sizeof(start_ota));
						start_ota.size_data = strlen("Update_comple");
						memcpy((uint8_t *)&start_ota.data, "Update_comple", start_ota.size_data);
						frame_work.header.msk_regs.len = start_ota.size_data + sizeof(start_ota.size_data);
						memcpy((uint8_t *)&frame_work.data, &start_ota, frame_work.header.msk_regs.len);
						frame_work.header.check_crc = Handle_SPIs.calcCRC(frame_work.data, frame_work.header.msk_regs.len);
						Serial1.write((uint8_t *)&frame_work, sizeof(frame_work.header) + frame_work.header.msk_regs.len);
						Serial1.flush(); // Ép phần cứng CHỜ đẩy xong từng bit cuối cùng ra dây TX
						FlagOTA = false;
						FlagWriteOTA = false;
						size_nhan = 0;
						delay(1000);
						ESP.restart();
					}
					else
					{
						memset((uint8_t *)&start_ota, 0, sizeof(start_ota));
						start_ota.size_data = strlen("Update_fail");
						memcpy((uint8_t *)&start_ota.data, "Update_fail", start_ota.size_data);
						frame_work.header.msk_regs.len = start_ota.size_data + sizeof(start_ota.size_data);
						memcpy((uint8_t *)&frame_work.data, &start_ota, frame_work.header.msk_regs.len);
						frame_work.header.check_crc = Handle_SPIs.calcCRC(frame_work.data, frame_work.header.msk_regs.len);
						Serial1.write((uint8_t *)&frame_work, sizeof(frame_work.header) + frame_work.header.msk_regs.len);
						Serial1.flush(); // Đảm bảo không mất gói hất bại
					}
				}
				else
				{
					memset((uint8_t *)&start_ota, 0, sizeof(start_ota));
					start_ota.size_data = strlen("MD5_fail");
					memcpy((uint8_t *)&start_ota.data, "MD5_fail", start_ota.size_data);
					frame_work.header.msk_regs.len = start_ota.size_data + sizeof(start_ota.size_data);
					memcpy((uint8_t *)&frame_work.data, &start_ota, frame_work.header.msk_regs.len);
					frame_work.header.check_crc = Handle_SPIs.calcCRC(frame_work.data, frame_work.header.msk_regs.len);
					Serial1.write((uint8_t *)&frame_work, sizeof(frame_work.header) + frame_work.header.msk_regs.len);
				}

				FlagWriteOTA = false;
				size_nhan = 0;
			}
		}

		else
		{
			/* Đọc data + 2 byte CRC16 từ U6 */
			size_t expect_len = size_nhan + 2; /* +2 cho CRC16 */
			size_t read_len = Serial1.readBytes(buffer_nhan, expect_len);
			if (read_len == expect_len)
			{
				/* Verify CRC16 per-chunk */
				uint16_t crc_rx = (uint16_t)buffer_nhan[size_nhan]
				                | ((uint16_t)buffer_nhan[size_nhan + 1] << 8);
				uint16_t crc_calc = ota_calcCRC16(buffer_nhan, size_nhan);
				
				if (crc_rx != crc_calc)
				{
					/* CRC sai → UART bị nhiễu, xả buffer rác và báo U6 gửi lại */
					dbg_spifs("CRC_FAIL: rx=0x%04X calc=0x%04X\n\r", crc_rx, crc_calc);
					while(Serial1.available()) Serial1.read(); /* Xả rác đọng */
					Serial1.print("CRC_FAIL\n");
					FlagWriteOTA = false;
				}
				else
				{
					Serial1.flush();
					
					if (this->writeFileOTA(buffer_nhan, size_nhan))
					{
						MD5Update(&md5_check, buffer_nhan, size_nhan);
						Serial1.print("FINISH\n");
					}
					else
					{
						Serial1.print("ERROR\n");
					}
					FlagWriteOTA = false;
				}
			}
			else
			{
				// Timeout đọc không đủ byte (bị nhiễu hoặc do U6 chậm)
				// Xả buffer rác rồi báo U6 gửi lại chunk này
				dbg_spifs("OTA read timeout: got %d, expect %d\n\r", read_len, expect_len);
				while(Serial1.available()) Serial1.read(); /* Xả rác đọng */
				Serial1.print("ERROR\n");
				FlagWriteOTA = false;
			}
		}
		delay(1);
	}
}

// perform the actual update from a given stream
uint8_t parametter::performUpdateFW(Stream &updateSource, size_t updateSize) // xử lý update OTA
{
	uint8_t ret = 0;
	if (Update.begin(updateSize))
	{
		size_t written = Update.writeStream(updateSource);
		if (written == updateSize)
		{
			dbg_spifs("Written : %d successfully", written);
		}
		else
		{
			dbg_spifs("Written only : %d / %d . Retry?", written, updateSize);
		}
		if (Update.end())
		{
			dbg_spifs("OTA done!");
			if (Update.isFinished())
			{
				dbg_spifs("Update successfully completed. Rebooting.");
				ret = 1;
			}
			else
			{
				dbg_spifs("Update not finished? Something went wrong!");
			}
		}
		else
		{
			dbg_spifs("Error Occurred. Error #: %s", Update.getError());
		}
	}
	else
	{
		dbg_spifs("Not enough space to begin OTA");
	}

	return ret;
}

uint8_t parametter::startUpdateFW(const char *path) // update FW
{
	uint8_t ret = 0;
	if (!this->SPIFFSbegin())
		return ret;

	File updateBin = SPIFFS.open(path, FILE_READ);
	if (updateBin)
	{
		if (updateBin.isDirectory())
		{
			dbg_spifs("Error, update.bin is not a file");
			updateBin.close();
			return 0;
		}
	}
	size_t updateSize = updateBin.size();
	dbg_spifs("size: %d", updateSize);
	if (updateSize > 0)
	{
		dbg_spifs("Try to start update");
		ret = this->performUpdateFW(updateBin, updateSize);
	}
	else
	{
		dbg_spifs("Error, file is empty");
		return 0;
	}

	updateBin.close();
	SPIFFS.end();
	return ret;
}

// char *parametter::sd_fix_file_name(char *file_name)
// {
// 	static char name_file[NAME_FILE_SIZE + 10];

// 	memset(name_file, 0, NAME_FILE_SIZE + 10);
// 	if (file_name[0] == '/')
// 		strcpy(name_file, file_name);
// 	else
// 		sprintf(name_file, "/%s", file_name);

// 	return name_file;
// 	// return file_name;
// }

// bool sd_read_block_file(bool status, char *path, uint8_t *data, size_t size)
// {
// 	static File file;
// 	char *re_rew_name;

// 	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
// 	if (sd_is_config_f == false && !SPIFFS.begin(true))
// 		return false;

// 	/*bật cờ báo bận*/
// 	while (sd_is_busy_f == true)
// 		;
// 	sd_is_busy_f = true;
// 	switch (status)
// 	{
// 	case false:
// 		file.close();
// 		re_rew_name = sd_fix_file_name(path);
// 		file = SPIFFS.open(re_rew_name);
// 		if (!file)
// 		{
// 			/*tắt cờ báo bận*/
// 			sd_is_busy_f = false;
// 			return false;
// 		}
// 	case true:
// 		if (file.available())
// 		{
// 			file.read(data, size);
// 			sd_is_busy_to.ToEUpdate(5);
// 			/*tắt cờ báo bận*/
// 			sd_is_busy_f = false;
// 			return true;
// 		}
// 		else
// 		{
// 			file.close();
// 			/*tắt cờ báo bận*/
// 			sd_is_busy_f = false;
// 			return false;
// 		}
// 	}
// 	/*tắt cờ báo bận*/
// 	sd_is_busy_f = false;
// }

// bool parametter::calculator_md5(char *name, char *md5)
// {
// 	/*đọc file ra check md5 trong sd card*/
// 	bool start = 0;
// 	size_t size_block = 1024;
// 	uint8_t *buff_data = (uint8_t *)malloc(size_block + 10);

// 	File file_pcs;
// 	file_pcs = SPIFFS.open(name);
// 	size_t file_size = file_pcs.size();

// 	// size_t file_size = sd_size_file(name);
// 	if(!file_size)
// 	{
// 		dbg_spifs("calculator md5 fail");
// 		return false;
// 	}

// 	MD5_CTX md5_check;
// 	MD5Init(&md5_check);

// 	while ((start = sd_read_block_file(start, name, buff_data, size_block)) == true)
// 	{
// 		delay(1);
// 		MD5Update(&md5_check, buff_data, size_block);
// 		/*kiểm tra kích thước còn lại. và out while thủ công*/
// 		if (size_block <= 0)
// 			break;
// 		/*tính kích thước còn lại*/
// 		file_size -= size_block;

// 		/*Nếu kích thước còn lại bé hơn 1 block cơ bản*/
// 		if (file_size < size_block && file_size >= 0)
// 			size_block = file_size;
// 	}

// 	free(buff_data);
// 	/*Tổng hợp check md5*/
// 	uint8_t md5_hash[16 + 1];
// 	uint8_t md5_hash_buff[32 + 1];

// 	MD5Final(md5_hash, &md5_check);
// 	for (uint8_t i = 0; i < 16; i += 1)
// 		sprintf((char *)&md5_hash_buff[i * 2], "%02X", md5_hash[i]);

// 	memcpy(md5, md5_hash_buff, 32);
// 	return true;
// }

parametter parametter_dw;
