#include "DataBase.h"
#include "handle_config.h"
#include "handle_ethernet.h"
#include "OTA.h"
#include "handle_mqtt.h"


void repost_accept(uint8_t accept)
{
    DynamicJsonDocument doc(512);

    doc["SerialID"] = Config_Device.Device.SerialID;
    doc["MessageID"] = 0;
    doc["CMDServerID"] = 0;
    doc["CMD"] = MQTT_CMD_ACC_SYNC_MASTER;
    doc["Packit ID"] = 0;
    doc["Access"] = accept;
    doc["isEnable"] = master_access.enable;
    doc["Serial_Master"] = master_access.Serial;
    doc["Timestamp"] = (uint32_t)master_access.Timestamp;
    doc["Reply"] = "STATUS ACCEPT";
    String output = "";
    serializeJson(doc, output);
    // free(doc);
    Mqtt_Handle.send_data(TopicDevice.c_str() ,output.c_str());
}
