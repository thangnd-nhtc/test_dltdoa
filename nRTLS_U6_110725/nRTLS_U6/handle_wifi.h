#ifndef __WIFI_HANDLE_H
#define __WIFI_HANDLE_H
#include "esp_wpa2.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <TimeOutEvent.h>

#include "handle_config.h"
#include "handle_logfile.h"

#define EW_WIFI_INTERVAL_RECONNECT 1 * 60000 //3p
#define EW_WIFI_RETRY_NUM_RECONNECT 15		 //15p reset device

// #define WIFIH_DEBUG(fmt, ...) Serial.printfln(">WIFI< " fmt, ##__VA_ARGS__)

extern TimeOutEvent ESPRebootTo;
extern bool eth_isconnect();

typedef enum
{
	ew_wifi_finish = 0,
	ew_wifi_init,
	ew_wifi_reinit,
	ew_wifi_deinit,
	ew_wifi_not_work,
} ew_wifi_config_enum;

class WifiHandle
{
private:
	uint8_t wifi_retry_connect = 0;
	bool WifiAP_is_run = false;
	bool WifiAP_Auto_turnoff = false;
	ew_wifi_config_enum wifi_config_e;
	TimeOutEvent wifi_config_to;
	TimeOutEvent wifi_interval_reconnect;

public:
	WifiHandle(/* args */);
	~WifiHandle();
	void ew_wifiAP_turnOn(void);
	void ew_wifiAP_turnOff(void);
	bool ew_wifiAP_is_run(void);
	void ew_wifi_config_status(ew_wifi_config_enum flag, int timeout);
	void ew_wifi_config(void);
	void ew_wifi_auto_process(ew_wifi_config_enum flag);
	void ew_wifi_handle(void);
};

extern WifiHandle Wifi_Handle;

#endif /* __WIFI_HANDLE_H */
