#ifndef __SPIs_HANDLE_H
#define __SPIs_HANDLE_H

#include "Arduino.h"
// #include <ESP32DMASPISlave.h>
#include <TimeOutEvent.h>
#include <functional>

#include "define.h"

#define SPI_SALVE_BUFF_SIZE_TX COMMUNICATION_LENGHT_BUFFER + 200
#define SPI_SALVE_BUFF_SIZE_RX COMMUNICATION_LENGHT_BUFFER + 1024

extern bool FlagreciverSPI;
class SPIsHandle
{
public:
	SPIsHandle(/* args */);
	~SPIsHandle();
	typedef std::function<void(uint8_t *, uint16_t)> HandlerFunction;

	void begin(void);
	void loop(void);
	void loopUART(void);
	void clearDMA();
	
	void tranmister(uint8_t *data, uint16_t length);
	void tranmisterDW(uint8_t *data, uint16_t length);
	void reciver_callback(HandlerFunction handle);

	uint16_t calcCRC(uint8_t *data, size_t size);

private:
	HandlerFunction rx_callback = NULL;

	uint8_t *tx_buf;
	
	uint8_t *rx_buf;
};

extern SPIsHandle Handle_SPIs;
#endif
