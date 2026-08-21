/*! ----------------------------------------------------------------------------
 * @file	deca_spi.c
 * @brief	SPI access functions
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */
#include "Arduino.h"
#include <SPI.h>
#include "deca_spi.h"
#include "deca_device_api.h"

#include "define.h"

SPIClass hSPI(HSPI);
SPISettings *spi_config;

void Set_Spi_Speed(uint32_t Speed)
{
	if (Speed == LF_SPI)
		spi_config = new SPISettings(Speed, MSBFIRST, SPI_MODE2);
	else
		spi_config = new SPISettings(Speed, MSBFIRST, SPI_MODE3);
}

void io_init(void)
{
	pinMode(SPI_DW_CS, OUTPUT);
	digitalWrite(SPI_DW_CS, HIGH);
	pinMode(DW_RESET, OUTPUT_OPEN_DRAIN);

	hSPI.begin(SPI_DW_CLK, SPI_DW_MISO, SPI_DW_MOSI, SPI_DW_CS);
}

int writetospi(uint16_t headerLength, const uint8_t *headerBuffer, uint32_t bodylength, const uint8_t *bodyBuffer)
{
	hSPI.beginTransaction(*spi_config);

	digitalWrite(SPI_DW_CS, LOW); //Write CS low to start
	// Send header offset
	hSPI.transfer((uint8_t *)headerBuffer, headerLength);

	// Send data to SPI
	hSPI.transfer((uint8_t *)bodyBuffer, bodylength);

	digitalWrite(SPI_DW_CS, HIGH); //Write CS low to start
	hSPI.endTransaction();

	return 0;
}
int readfromspi(uint16_t headerLength, const uint8_t *headerBuffer, uint32_t readlength, uint8_t *readBuffer)
{
	hSPI.beginTransaction(*spi_config);

	digitalWrite(SPI_DW_CS, LOW); //Write CS low to start
	// Send header offset
	hSPI.transfer((uint8_t *)headerBuffer, headerLength);

	// Send data to SPI
	for (uint16_t i = 0; i < readlength; i++)
		readBuffer[i] = hSPI.transfer(0x55);

	digitalWrite(SPI_DW_CS, HIGH); //Write CS low to start
	hSPI.endTransaction();

	return 0;
}

void deca_Reset(void)
{
	io_init();

	digitalWrite(DW_RESET, LOW);
	delay(1);
	digitalWrite(DW_RESET, HIGH);
	delay(1);
}

void deca_Test(void)
{
	uint8_t Buf[10];
	uint8_t Header = 0;
	readfromspi(1, &Header, 4, Buf);
	dbg_dw("Test: %02X%02X%02X%02X", Buf[0], Buf[1], Buf[2], Buf[3]);
}
