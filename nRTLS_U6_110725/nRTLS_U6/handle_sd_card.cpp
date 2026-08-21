#include "handle_sd_card.h"

SDHandle::SDHandle() {
	this->isBussy_TO = new TimeOutEvent(0);
	}
SDHandle::~SDHandle() {}

SPIClass Hard_SPI(HSPI);

// SPIClass H_SPI(HSPI);

bool SDHandle::wait_bussy(void)
{
	while (this->isBussy_f == true && this->isBussy_TO->ToERemain())
		;
}
void SDHandle::set_bussy(uint32_t timeout)
{
	this->isBussy_f = true;
	this->isBussy_TO->ToEUpdate(timeout);
}
void SDHandle::release_bussy(void)
{
	this->isBussy_TO->ToEDisable();
	this->isBussy_f = false;
}
bool SDHandle::begin(void)
{
	//bật cờ báo bận
	this->wait_bussy();
	this->set_bussy(100);


	dbg_sd("SD Card is config..");


  //pinMode(SD_CARD_POWER, OUTPUT);
  //digitalWrite(SD_CARD_POWER, LOW);
	delay(100);
	// Hard_SPI.end();
	Hard_SPI.begin(SPI_COM_CLK,SPI_COM_MISO,SPI_COM_MOSI,SD_CARD_CS);

	// SD.end();
	if (SD.begin(SD_CARD_CS, Hard_SPI))
	{
		dbg_sd("SD Card is ready");
		this->isReady_f = true;

		//Khởi tạo file để làm mốc so sánh
		this->isBussy_f = false;
		// if (sd_check_file(STATUS_SD_FILE) == false)
		// 	sd_create_file(STATUS_SD_FILE);
	}
	else
	{
		dbg_sd("SD Card not config");
		this->isReady_f = false;
	}
	// Tắt cờ báo bận
	this->release_bussy();
	
}



SDHandle SD_Handle;
