#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "Arduino.h"
#include <ArduinoJson.h>

// bool WebAuthCheck(char *User, char *Pass);
void WebServerInit(void);
void Webserverloop();
String getContentType(String filename);
void handleFileList();
bool handleFileRead(String path);
bool apply_beacon_config_json(DynamicJsonDocument &djbpo);
String bytesToHex(uint8_t *bytes, int len);

extern String current_target_tag_id_str;
extern String g_beacon_ack_status;
extern uint64_t g_beacon_ack_bits;
extern volatile bool g_base_bcast_twr_active;
void restore_bcast_twr_after_boot();
void sendmqtt_fragment_status(uint64_t status);

#endif
