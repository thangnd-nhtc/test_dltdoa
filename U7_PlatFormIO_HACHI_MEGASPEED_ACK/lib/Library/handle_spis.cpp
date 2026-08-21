#include "handle_spis.h"
#include <ESP32DMASPIMaster.h>
// #include "freertos/semphr.h"

// ESP32DMASPI::Slave slave;
ESP32DMASPI::Master master;
volatile bool FlagsendSPI = false;
// volatile uint32_t dem1 = 0;
// volatile uint32_t dem2 = 0;
uint16_t len = 0;
TimeOutEvent checktimeSPI(0);
uint8_t* rx_buf2;
uint8_t *spi_master_tx_buf;
bool FlagreciverSPI = false;
volatile bool status = false;
// TaskHandle_t task_send;
// TaskHandle_t task_reviver;

// SemaphoreHandle_t xMutex2 = NULL;

SPIsHandle::SPIsHandle(/* args */)
{
	 
	spi_master_tx_buf = master.allocDMABuffer(SPI_SALVE_BUFF_SIZE_TX);
	// rx_buf2 = slave.allocDMABuffer(SPI_SALVE_BUFF_SIZE_RX);
	// this->tx_buf = slave.allocDMABuffer(SPI_SALVE_BUFF_SIZE_TX);
}

SPIsHandle::~SPIsHandle() {}


void SPIsHandle::clearDMA()
{
	// memset(&this->tx_buf,0,SPI_SALVE_BUFF_SIZE_TX + 1024);
	// memset(&this->rx_buf,0,SPI_SALVE_BUFF_SIZE_RX);
}

// void loop_send(void *arg)
// {
// 		// FlagsendSPI = true;
// 		// vTaskDelete(task_reviver);
// 		xSemaphoreTake(xMutex2, 1); 
// 		slave.queue(rx_buf2,tx_buf2, SPI_SALVE_BUFF_SIZE_TX);

// 		xSemaphoreGive(xMutex2);
// 		FlagsendSPI = false;	
// 		FlagreciveSPI = true;
// 		vTaskDelete(task_send);
// }

void loop_reciver(void *arg)
{
	// while(1)
	// {
	// 	if(FlagreciverSPI)
	// 	{
	// 		slave.queue(rx_buf2,tx_buf2, SPI_SALVE_BUFF_SIZE_TX);
	// 	}
	// }
	// }
}

void SPIsHandle::begin(void)
{
    master.setDataMode(SPI_MODE3);
    // master.setFrequency(SPI_MASTER_FREQ_8M); // too fast for bread board...
    master.setFrequency(4000000);
    master.setMaxTransferSize(255);
    // master.setDMAChannel(2); // 1 or 2 only
    master.setQueueSize(1);  // transaction queue size
    // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12
    // master.begin();  // default SPI is HSPI
    // master.begin(HSPI,14,12,13,4);  // default SPI is HSPI
    master.begin(VSPI, SPI_COM_CLK, SPI_COM_MISO, SPI_COM_MOSI, SPI_COM_CS); // default SPI is HSPI
/*
	slave.setDataMode(SPI_MODE0);
	slave.setMaxTransferSize(SPI_SALVE_BUFF_SIZE_TX);
	slave.setDMAChannel(2); // 1 or 2 only
	slave.setQueueSize(80);	// transaction queue size
	slave.setTimeout(portMAX_DELAY);
	// begin() after setting
	// HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12
	slave.begin(VSPI, SPI_COM_CLK, SPI_COM_MISO, SPI_COM_MOSI, SPI_COM_CS); // default SPI is HSPI
	// xTaskCreatePinnedToCore(loop_reciver, "spi_loop_reciver", 2049, NULL, 2, &task_reviver, 1);
	// slave.end();
	*/
}

void SPIsHandle::reciver_callback(HandlerFunction handle)
{
	this->rx_callback = handle;
}

void SPIsHandle::tranmister(uint8_t *data, uint16_t length)
{
	// memset((uint8_t *)spi_master_tx_buf, 0, SPI_SALVE_BUFF_SIZE_TX);
    // memcpy((uint8_t *)spi_master_tx_buf, data, length);
	// char buffer_send[length];
	// memset(buffer_send,NULL,length);
	// memcpy((uint8_t*)buffer_send,data,length);
    // Serial.printf("\r\n send:");
    // for (unsigned int i = 0; i < len; i++)
    // 	Serial.printf("%d ", spi_master_tx_buf[i]);
    // Serial.println("");
    master.transfer(data,length);
    // memset(spi_master_tx_buf, 0, BUFFER_SIZE);
}

void SPIsHandle::tranmisterDW(uint8_t *data, uint16_t length)
{
	// memset((uint8_t*)&tx_buf2[0],0,SPI_SALVE_BUFF_SIZE_TX);
	// memcpy((uint8_t*)&tx_buf2[0], data, (length < SPI_SALVE_BUFF_SIZE_TX) ? length : SPI_SALVE_BUFF_SIZE_TX);
	// digitalWrite(SPI_COM_DRDY, HIGH);
	// delayMicroseconds(1);
	// digitalWrite(SPI_COM_DRDY, LOW);
	// slave.queue(rx_buf2,tx_buf2, SPI_SALVE_BUFF_SIZE_TX);
}

void SPIsHandle::loop(void)
{

	// slave.queue(rx_buf2,tx_buf2, SPI_SALVE_BUFF_SIZE_TX);

	// if(slave.available())
	// {
	// 	int length_rx = slave.size();
	// 	if (length_rx > 0 && length_rx < SPI_SALVE_BUFF_SIZE_RX && this->rx_callback != NULL)
	// 	{
	// 		this->rx_callback(rx_buf2, length_rx);
	// 		// this->clearDMA();
	// 		memset(rx_buf2,0,SPI_SALVE_BUFF_SIZE_RX);
	// 	}
	// 	slave.pop();
	// }
}

// void SPIsHandle::loopUART(void)
// {
// 	if(Serial1.available())
// 	{
// 	  	delay(100);
// 	    size_t length_rx = Serial1.readBytes(rx_buf2,Serial1.available());
// 		if(this->rx_callback != NULL)
// 		this->rx_callback(rx_buf2, length_rx);
// 	}

// }

uint16_t SPIsHandle::calcCRC(uint8_t *data, size_t size)
{
	if (size <= SPI_SALVE_BUFF_SIZE_TX)
	{
		uint16_t CrcPoly_U16 = 0x8408;
		uint16_t Crc_U16 = 0;
		uint8_t j, i_bits, Carry_U8;

		for (j = 0; j < size; j++)
		{
			Crc_U16 = Crc_U16 ^ data[j];
			for (i_bits = 0; i_bits < 8; i_bits++)
			{
				Carry_U8 = Crc_U16 & 1;
				Crc_U16 = Crc_U16 / 2;
				if (Carry_U8)
					Crc_U16 = Crc_U16 ^ CrcPoly_U16;
			}
		}
		return Crc_U16;
	}
	// else
	// 	dbg_spi("Data lenght over long");

	return 0xFFFF;
}




SPIsHandle Handle_SPIs;
