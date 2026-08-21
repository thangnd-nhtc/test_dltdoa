#ifndef __SPI_HANDLE_H
#define __SPI_HANDLE_H

#include "Arduino.h"
#include "DataBase.h"
#include "define.h"
#include <functional>


#define SIZE_TCP_BUFFER 255
static const uint32_t BUFFER_SIZE = 1024 + SIZE_TCP_BUFFER;
extern uint16_t FlagRead;
extern TimeOutEvent SPI_TimeCheck;

// extern volatile bool flag;
extern volatile bool flagsend;
extern bool FlagcheckLed;
extern bool OTAstart;
#pragma once

class handle_spi {

public:
  handle_spi(/* args */);
  ~handle_spi();
  typedef std::function<void(uint8_t *, uint16_t)> HandlerFunction;
  void reciver_callback(HandlerFunction handle);

  void begin();
  void loop(void);
  void enable_interrup();
  void disable_interrup();
  void check_status_conncet();
  void reset();
  uint16_t calcCRC(uint8_t *data, size_t size);
  void masterTransfer(uint8_t *bufferSend, uint32_t len);
  void masterRead(uint8_t *bufferread);
  uint16_t masterGet(uint8_t *address, uint8_t *bufferRead, uint32_t timeout);
  void addBufferDMA(uint8_t *Buf, uint16_t Length);
  uint32_t Buff_is_available(void);
  uint8_t readBufferDMA(uint8_t *data, uint32_t Length);
  void setConfigDW(void);
  void readConfigDW(void);
  void read_led();
  void readFragmentStatus();
  void readISPVersion();

  // spi beacon
  void setConfigBeacon();

private:
  /* data */
  com_frame_t tx_frame;
  HandlerFunction rx_callback = NULL;
};

extern handle_spi SPI_master;
#endif
