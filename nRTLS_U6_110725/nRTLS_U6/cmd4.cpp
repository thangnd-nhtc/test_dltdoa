#include "DataBase.h"
#include "handle_config.h"
#include "handle_ethernet.h"
#include "OTA.h"
#include "handle_mqtt.h"


void UpdateConfig()
{
    DynamicJsonDocument doc(1024);

    doc["Serial"] = Config_Device.Device.Serial;
    doc["SerialID"] = Config_Device.Device.SerialID;
    doc["ServerMQTT"] = Config_Internet.MQTT.Server;
    doc["CMD"] = MQTT_CMD_SERVER_DATA;
    doc["CMDServerID"] = 0;
    doc["Packit ID"] = 0;

    doc["PortMQTT"] = Config_Internet.MQTT.Port;
    doc["UserMQTT"] = Config_Internet.MQTT.User;
    doc["PassMQTT"] = Config_Internet.MQTT.Pass;
    doc["Topicserver"] = Config_Internet.MQTT.TopicDevice;
    doc["Topicdevice"] = Config_Internet.MQTT.TopicDevice;
    doc["ServerTCP"] = Config_Internet.TCP.Server;
    doc["PortTCP"] = Config_Internet.TCP.Port;
    doc["ServerFTP"] = Config_Internet.FTP.Server;
    doc["PortFTP"] = Config_Internet.FTP.Port;
    doc["UserFTP"] = Config_Internet.TCP.User;
    doc["PassFTP"] = Config_Internet.TCP.Pass;
    doc["AuthUser"] = Config_Internet.Auth.User;
    doc["AuthPass"] = Config_Internet.Auth.Pass;

    doc["D_Hardware"] = Config_Device.Version_decawave.HW_Ver;
    doc["D_Firmware"] = Config_Device.Version_decawave.FW_Ver;
    doc["I_Hardware"] = I_HARDWARE_VERSION;
    doc["I_Firmware"] = I_FIRMWARE_VERSION;

    doc["AuthPass"] = Config_Internet.Auth.Pass;
    doc["AuthPass"] = Config_Internet.Auth.Pass;

    doc["Led_P"] = led_status_t.power;
    doc["Led_DW"] = led_status_t.decawave;
    doc["Led_I"] = led_status_t.internet;

    // doc["SSIDWifiSTA"] = Config_Internet.Wifi_STA.Ssid;
    // doc["PassWifiSTA"] = Config_Internet.Wifi_STA.Pass;
    doc["SSIDWifiAP"] = Config_Internet.Wifi_AP.Ssid;
    doc["PassWifiAP"] = Config_Internet.Wifi_AP.Pass;

    char ip[20];
    char gw[20];
    char sn[20];
    char dns[20];

    if (eth_connected)
    {
        doc["ConnectMode"] = "E";
        sprintf(ip, "%s", Config_Internet.Ethernet.Ip.toString().c_str());
        sprintf(gw, "%s", Config_Internet.Ethernet.Gw.toString().c_str());
        sprintf(sn, "%s", Config_Internet.Ethernet.Sn.toString().c_str());
        sprintf(dns, "%s", Config_Internet.Ethernet.Dns.toString().c_str());
        doc["IP"] = ip;
        doc["GW"] = gw;
        doc["Sn"] = sn;
        doc["DNS"] = dns;
    }

    else if (wifi_connected)
    {
        doc["ConnectMode"] = "W";
        sprintf(ip, "%s", Config_Internet.Wifi_STA.Ip.toString().c_str());
        sprintf(gw, "%s", Config_Internet.Wifi_STA.Gw.toString().c_str());
        sprintf(sn, "%s", Config_Internet.Wifi_STA.Sn.toString().c_str());
        sprintf(dns, "%s", Config_Internet.Wifi_STA.Dns.toString().c_str());
        doc["IP"] = ip;
        doc["GW"] = gw;
        doc["Sn"] = sn;
        doc["DNS"] = dns;
    }
    else
    {
        doc["ConnectMode"] = "N";
        doc["IP"] = "0.0.0.0";
        doc["GW"] = "0.0.0.0";
        doc["Sn"] = "0.0.0.0";
        doc["DNS"] = "0.0.0.0";
    }

    doc["EMAC"] = Config_Internet.Ethernet.Mac;
    doc["WMAC"] = Config_Internet.Wifi_STA.Mac;

    String output = "";
    serializeJson(doc, output);
    Mqtt_Handle.send_data(TopicDevice.c_str(), output.c_str());
}
