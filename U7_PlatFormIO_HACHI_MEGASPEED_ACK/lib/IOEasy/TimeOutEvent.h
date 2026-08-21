
#ifndef	_TIMEOUTEVENT_H
#define _TIMEOUTEVENT_H
#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

class TimeOutEvent
{
	public:
		TimeOutEvent(uint32_t Tio = 0);
		void ToEUpdate(uint32_t Tio);
		bool ToEExpired(void);
		void ToEDisable(void); 
		bool ToEGetStatus(void);
		uint32_t ToERemain(void);

	protected:
		uint32_t TSta;  //Time Start
		uint32_t Tio;   //Timeout
		bool En;
};

#endif
