#include "DataBase.h"
#include "ESP_aes.h"
#include "handle_config.h"
#include "handle_mqtt.h"
#include "web_server.h"

extern beacon_cfg_t g_beacon_cfg;

void sendmqtt_distance_cmd10(uint32_t BaseID, uint32_t distance) {
  DynamicJsonDocument doc(512);

  doc["SerialID"] = Config_Device.Device.SerialID;
  doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
  doc["CMD"] = 10;

  String tag_id_str = current_target_tag_id_str;
  if (tag_id_str == "") {
    tag_id_str = bytesToHex(g_beacon_cfg.val_id_last, 5);
  }

  bool is_decimal_id = (tag_id_str.length() > 0);
  for (size_t i = 0; i < tag_id_str.length(); i++) {
    if (!isDigit(tag_id_str[i])) {
      is_decimal_id = false;
      break;
    }
  }

  if (is_decimal_id) {
    doc["TagID"] = (uint32_t)strtoul(tag_id_str.c_str(), nullptr, 10);
  } else {
    doc["TagID"] = tag_id_str;
  }
  doc["BaseID"] = BaseID;

  if (distance == 0xFFFFFFFF) {
    doc["Distance"] = -1;
  } else {
    doc["Distance"] = distance;
  }
  doc["Reply"] = "OK";

  String output = "";
  serializeJson(doc, output);
  Mqtt_Handle.send_data(TopicDevice.c_str(), output.c_str());
}