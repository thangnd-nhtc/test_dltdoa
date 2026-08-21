#include "handle_config.h"
#include "handle_SPIFFS.h"
#include "ArduinoJson.h"

/**
 * CHƯƠNG TRÌNH DÀNH CHO CẤU HÌNH DEVICE
 * 1. Thông tin công ty
 * 2. Thông tin device
 * 3. Thông số set time
 * */

ConfigDevice::ConfigDevice() {}
ConfigDevice::~ConfigDevice() {}

void ConfigDevice::checkBaseConfig()
{
    String dataInfor = _handle_SPIFFS.readInformations(BASECONFIG);
    DynamicJsonDocument doc(1536);

    DeserializationError error = deserializeJson(doc, dataInfor);

    if (error)
    {
        dbg_sd("deserializeJson() baseconfig failed: %s", error.c_str());
        return;
    }

    strcpy(Config_Device.Device.Serial, doc["Serial"]); // 1
    dbg_sd("Serial:%s", Config_Device.Device.Serial);
    // uint32_t chipId;
    // for(int i=0; i<17; i=i+8) {
    //   chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    // }
    // chipId = chipId & 0xFFFF;
    Config_Device.Device.SerialID = doc["SerialID"];
    dbg_sd("SerialID:%lu", Config_Device.Device.SerialID);
    strcpy(Config_Internet.MQTT.Server, doc["ServerMQTT"]);
    dbg_sd("ServerMQTT:%s", Config_Internet.MQTT.Server);
    Config_Internet.MQTT.Port = doc["PortMQTT"];
    dbg_sd("PortMQTT:%d", Config_Internet.MQTT.Port);
    strcpy(Config_Internet.MQTT.User, doc["UserMQTT"]);
    strcpy(Config_Internet.MQTT.Pass, doc["PassMQTT"]);
    strcpy(Config_Internet.MQTT.Server, doc["ServerMQTT"]);
    strcpy(Config_Internet.MQTT.TopicDevice, doc["Topicdevice"]);
    strcpy(Config_Internet.MQTT.TopicServer, doc["Topicserver"]);

    strcpy(Config_Internet.TCP.Server, doc["ServerTCP"]);
    Config_Internet.TCP.Port = doc["PortTCP"];

    strcpy(Config_Internet.FTP.Server, doc["ServerFTP"]);
    Config_Internet.FTP.Port = doc["PortFTP"];
    strcpy(Config_Internet.FTP.User, doc["UserFTP"]);
    dbg_sd("UserFTP:%s", Config_Internet.FTP.User);
    strcpy(Config_Internet.FTP.Pass, doc["PassFTP"]);
    dbg_sd("PassFTP:%s", Config_Internet.FTP.Pass);

    strcpy(Config_Internet.Auth.User, doc["AuthUser"]);
    strcpy(Config_Internet.Auth.Pass, doc["AuthPass"]);

    strcpy(Config_Internet.Wifi_STA.Ssid, doc["SSIDWifiSTA"]);
    dbg_sd("SSIDWifiSTA:%s", Config_Internet.Wifi_STA.Ssid);
    strcpy(Config_Internet.Wifi_STA.Pass, doc["PassWifiSTA"]);
    dbg_sd("PassWifiSTA:%s", Config_Internet.Wifi_STA.Pass);
    strcpy(Config_Internet.Wifi_AP.Ssid, doc["SSIDWifiAP"]);
    strcpy(Config_Internet.Wifi_AP.Pass, doc["PassWifiAP"]);
}

void ConfigDevice::checkDHCPconfig()
{
    String dhcpdata = _handle_SPIFFS.readInformations(DHCPCONFIG);
    DynamicJsonDocument doc(1024);

    DeserializationError error = deserializeJson(doc, dhcpdata);

    if (error)
    {
        dbg_sd("deserializeJson() DHCPconfig failed: %s", error.c_str());
        return;
    }

    Config_Internet.DHCP_setting.DHCPEn = doc["DhcpMode"];
    strcpy(Config_Internet.DHCP_setting.Ip, doc["IP"]);
    dbg_sd("DHCP_IP:%s", Config_Internet.DHCP_setting.Ip);
    strcpy(Config_Internet.DHCP_setting.Gw, doc["GW"]);
    dbg_sd("DHCP_GW:%s", Config_Internet.DHCP_setting.Gw);
    strcpy(Config_Internet.DHCP_setting.Sn, doc["Sn"]);
    dbg_sd("DHCP_SN:%s", Config_Internet.DHCP_setting.Sn);
    strcpy(Config_Internet.DHCP_setting.Dns, doc["DNS"]);
    dbg_sd("DHCP_DNS:%s", Config_Internet.DHCP_setting.Dns);
}
/**
 * CHƯƠNG TRÌNH DÀNH CHO CẤU HÌNH DEVICE
 * 1. cấu hình cho LAN
 * 2. cấu hình cho wifi mode sta
 * 3. cấu hình cho wifi enterprise
 * 4. cấu hình cho wifi mode ap
 * 5. cấu hình đăng nhập web
 * 6. cấu hình controler server cho giao tiếp MQTT
 * 7. cấu hình server cho ftp
 * 8. cấu hình server cho ntp
 * 9. cấu hình thông tin đăng nhập MQTT
 * */
ConfigInternet::ConfigInternet() {}
ConfigInternet::~ConfigInternet() {}

ConfigDevice Config_Device;
ConfigInternet Config_Internet;
