#include "DataBase.h"
#include "ESP_aes.h"
#include "handle_config.h"
#include "handle_mqtt.h"
#include "web_server.h"

extern beacon_cfg_t g_beacon_cfg;

void sendmqtt_request_data(request_data_t *req) {
  DynamicJsonDocument doc(1024);
  doc["SerialID"] = Config_Device.Device.SerialID;
  doc["CMD"] = 97;

  if (current_target_tag_id_str != "") {
    doc["TagID"] = current_target_tag_id_str;
  } else {
    doc["TagID"] = bytesToHex(g_beacon_cfg.val_id_last, 5);
  }

  JsonObject timer = doc.createNestedObject("Timer");
  timer["Motion"] = req->timer.send_tag_motion;
  timer["Stand"] = req->timer.send_tag_stand;
  timer["Sleep1"] = req->timer.send_tag_sleep1;
  timer["Sleep2"] = req->timer.send_tag_sleep2;
  timer["Sleep3"] = req->timer.send_tag_sleep3;
  timer["Mode1"] = req->timer.sleep_mode1;
  timer["Mode2"] = req->timer.sleep_mode2;
  timer["Mode3"] = req->timer.sleep_mode3;
  timer["BattDef"] = req->timer.batt_default;
  timer["BattHigh"] = req->timer.batt_high;
  timer["BattInc"] = req->timer.batt_increase;
  timer["BattDec"] = req->timer.batt_decrease;

  JsonObject config = doc.createNestedObject("Config");
  config["Chan"] = req->config.uwb_chan;
  config["Plen"] = req->config.uwb_plen;
  config["PAC"] = req->config.uwb_pac;
  config["TxCode"] = req->config.uwb_txcode;
  config["RxCode"] = req->config.uwb_rxcode;
  config["SFDType"] = req->config.uwb_sfdtype;
  config["DataRate"] = req->config.uwb_datarate;
  config["PHRMode"] = req->config.uwb_phrmode;
  config["PHRRate"] = req->config.uwb_phrrate;
  config["SFDTO"] = req->config.uwb_sfdto;
  config["STSMode"] = req->config.uwb_stsmode;
  config["STSLen"] = req->config.uwb_stslen;
  config["PDOA"] = req->config.uwb_pdoa;

  doc["State"] = req->current_state;

  JsonObject id_tag = doc.createNestedObject("ID_Tag");
  id_tag["ID"] = bytesToHex(req->id_tag.id, 5);

  String output = "";
  serializeJson(doc, output);
  Mqtt_Handle.send_data(TopicDevice.c_str(), output.c_str());
  dbg_main("Sent MQTT Request Data (CMD 97)");
}
