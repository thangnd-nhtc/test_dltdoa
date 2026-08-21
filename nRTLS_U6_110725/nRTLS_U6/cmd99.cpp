#include "DataBase.h"
#include "ESP_aes.h"
#include "handle_config.h"
#include "handle_mqtt.h"
#include "web_server.h"

extern beacon_cfg_t g_beacon_cfg;

void sendmqtt_fragment_status(uint64_t status) {
  DynamicJsonDocument doc(512);
  doc["SerialID"] = Config_Device.Device.SerialID;
  doc["CMD"] = 99; // Giả sử CMD 99 cho báo cáo cấu hình BLE fragment

  if (current_target_tag_id_str != "") {
    doc["TagID"] = current_target_tag_id_str;
  } else {
    doc["TagID"] = bytesToHex(g_beacon_cfg.val_id_last, 5);
  }

  // Chuyển 64-bit status thành chuỗi Hex để tránh JSON bị mất dữ liệu
  char status_str[32];
  sprintf(status_str, "%08X%08X", (uint32_t)(status >> 32),
          (uint32_t)(status & 0xFFFFFFFF));
  doc["FragmentStatus"] = status_str;

  if (status == 0) {
    doc["Reply"] = "PENDING";
  } else {
    doc["Reply"] = "ACKED";
  }
  String output = "";
  serializeJson(doc, output);
  Mqtt_Handle.send_data(TopicDevice.c_str(), output.c_str());
}
