#include "DataBase.h"
#include "OTA.h"
#include "handle_config.h"
#include "handle_ethernet.h"
#include "handle_mqtt.h"

void sendmqtt_distance(uint32_t BaseID, uint32_t distance) {
  DynamicJsonDocument doc(512);

  // doc["SerialID"] = MQTT_Exchange.SerialID; //Hieu  2024011
  //  doc["MessageID"] = MQTT_Exchange.PackitID;
  doc["SerialID"] = Config_Device.Device.SerialID; // Hieu  2024011
  doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
  doc["CMD"] = MQTT_CMD_TWO_WAY;
  // doc["Packit ID"] = 0;
  doc["BaseID"] = BaseID;
  doc["Distance"] = distance;
  doc["Reply"] = "OK";
  String output = "";
  serializeJson(doc, output);
  // free(doc);
  Mqtt_Handle.send_data(TopicDevice.c_str(), output.c_str());
}
