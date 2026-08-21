#ifndef __MQTT_HANDLE_H
#define __MQTT_HANDLE_H

#include <WiFi.h>
#include <PubSubClient.h>

#include "define.h"

extern String TopicServer;
extern String TopicDevice;
// extern MQTT_Exchange_str MQTT_Exchange;

class MqttHandle
{
public:
	MqttHandle(/* args */);
	~MqttHandle();

	bool mqtt_isconnect(void);
    void mqtt_setup();
	void mqtt_loop();
	void mqtt_send_data(uint8_t cmd, char *AES_key, String Data);
	void send_data(const char *topic,const char * data);

private:
	WiFiClient MQTTClient;
	PubSubClient *mqtt_client;

	uint32_t Buff_is_available(void);
	uint8_t readBufferSend(uint8_t *data, uint32_t Length);
	void mqtt_checksend();
	bool isConnected = false;
	bool MQTT_isConnected;
};

extern MqttHandle Mqtt_Handle;
void mqtt_send_isp_version(); // ISP version CMD 24
#endif // __MQTT_HANDLE_H
