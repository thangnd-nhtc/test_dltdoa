/*! ----------------------------------------------------------------------------
 * @file	deca_spi.h
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

#ifndef _DECA_SPI_H_
#define _DECA_SPI_H_

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "deca_types.h"

#define HF_SPI   20000000//20E6   // 20MHz
#define LF_SPI   2000000//2E6    // 2MHz

void Deca_Test(void);
void Set_Spi_Speed(uint32_t Speed);

#endif /* _DECA_SPI_H_ */
