#ifndef __HANDLE_CONFIG_H
#define __HANDLE_CONFIG_H

#include <IPAddress.h>
#include "define.h"

#define BASECONFIG   "/baseconfig.txt"
#define DHCPCONFIG   "/dhcpconfig.txt"
#define BEACONCONFIG  "/beaconconfig.txt"
class ConfigDevice
{
public:
	ConfigDevice(/* args */);
	~ConfigDevice();

	void checkBaseConfig();
	void checkDHCPconfig();
	struct
	{
		char Developer[64] = "ONTRAK HACHI";
		char Addrress[64] = "18A4";
		char Model[64] = "ONTRAK HACHI-PoE/Wifi";
		char Serial[64] = "ONTRAK HACHI";
		char HostName[64] = "ONTRAK HACHI";
		uint32_t SerialID = 2109;
	} Device;

	struct
	{
		char FW_Ver[10];
		char HW_Ver[10];
	} Version_decawave;

	struct
	{
		uint8_t Config = 1;
		uint8_t Format = 2;
		int8_t TimeZone = 2;
		struct
		{
			uint8_t Week = 1;
			uint8_t DaW = 1;
			uint8_t Month = 1;
			uint8_t Hour = 1;
			uint8_t TZone = 1;
		} DSTs;
		struct
		{
			uint8_t Week = 1;
			uint8_t DaW = 1;
			uint8_t Month = 1;
			uint8_t Hour = 1;
			uint8_t TZone = 1;
		} STDs;
	} Time;
};

class ConfigInternet
{
public:
	ConfigInternet(/* args */);
	~ConfigInternet();

	struct
	{
		uint8_t DHCPEn = 0;
		char Ip[16];  // = "192.168.1.184";
		char Gw[16];  // = "192.168.1.1";
		char Sn[16];  // = "0.0.0.0";
		char Dns[16]; // = "0.0.0.0";
	} DHCP_setting;
	struct
	{
		// bool DHCPEn = false;
		IPAddress Ip;  // = "192.168.1.184";
		IPAddress Gw;  // = "192.168.1.1";
		IPAddress Sn;  // = "0.0.0.0";
		IPAddress Dns; // = "0.0.0.0";
		char Mac[32] = "";
	} Ethernet;
	struct
	{
		char Ssid[64] = "ONTRAK HACHI"; //  ontrak_01 Luscas
		char Pass[64] = "ONTRAK HACHI"; //nhtc130nmh  Lucas1505
		// bool DHCPEn = false;
		IPAddress Ip;  // = "0.0.0.0";
		IPAddress Gw;  // = "0.0.0.0";
		IPAddress Sn;  // = "0.0.0.0";
		IPAddress Dns; // = "0.0.0.0";
		char Mac[32] = "";
	} Wifi_STA;
	struct
	{
		bool Enable = false;
		char UserName[64];
	} Enterprise;
	struct
	{
		bool Enable = false;
		char Ssid[64] = "ONTRAK HACHI";
		char Pass[64] = "12345678";
		IPAddress Ip; // = "192.168.4.1";
		IPAddress Gw; // = "192.168.4.1";
		IPAddress Sn; // = "255.255.255.0";
	} Wifi_AP;
	struct
	{
		char User[32] = "admin";
		char Pass[32] = "admin";
	} Auth;
	struct
	{
		char Server[64] = "";
		uint16_t Port = 1883;
	} Controler;
	struct
	{
		char Server[64] = "118.69.65.208"; //ftp://118.69.65.208/BaseStation/
		uint16_t Port = 21;
		char User[64] = "ota_clock";
		char Pass[64] = "Nhtc18a$";
	} FTP;
	struct
	{
		char Server[64] = "118.69.65.208";
		uint16_t Port = 123;
	} NTP;
	struct
	{
		char Server[64] = "118.69.65.208";
		char User[64] = "iiClock";
		uint16_t Port = 1883;
		char Pass[64] = "gclk@2109";
		char TopicServer[64] = "server";
		char TopicDevice[64] = "device";
	} MQTT;
	struct
	{
		char Server[64] = "192.168.22.185";  //192.168.22.193
		uint16_t Port = 7777;
		char User[64] = "ota_clock";
		char Pass[64] = "nhtc130nmh";
	} TCP;
};

extern ConfigDevice Config_Device;
extern ConfigInternet Config_Internet;


#endif // __HANDLE_CONFIG_H
