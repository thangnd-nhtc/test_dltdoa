#include "handle_wifi.h"

WifiHandle::WifiHandle() {}
WifiHandle::~WifiHandle() {}
TimeOutEvent ESPRebootTo;

void WifiHandle::ew_wifi_config_status(ew_wifi_config_enum flag, int timeout)
{
	wifi_config_e = flag;
	wifi_config_to.ToEUpdate(timeout);
	dbg_wifi("reciver flag: %d, timeout: %d", flag, timeout);
}

void WifiHandle::ew_wifiAP_turnOn(void)
{
	WifiAP_Auto_turnoff = false;
}

void WifiHandle::ew_wifiAP_turnOff(void)
{
	WifiAP_Auto_turnoff = true;
}

bool WifiHandle::ew_wifiAP_is_run(void)
{
	return WifiAP_is_run;
}

#include <HTTPClient.h>
void WifiHandle::ew_wifi_config(void)
{
	/*Nếu set ip cứng*/
	if (Config_Internet.DHCP_setting.DHCPEn == 0)
	{
		dbg_wifi("Mode disable dhcp");
		WiFi.mode(WIFI_OFF);
		IPAddress Eip;
		WiFi.config(Eip.fromString(Config_Internet.DHCP_setting.Ip),
					Eip.fromString(Config_Internet.DHCP_setting.Gw),
					Eip.fromString(Config_Internet.DHCP_setting.Sn),
					Eip.fromString(Config_Internet.DHCP_setting.Dns));
		// WiFi.config(Config_Internet.Wifi_STA.Ip, Config_Internet.Wifi_STA.Gw, Config_Internet.Wifi_STA.Sn, Config_Internet.Wifi_STA.Dns);
	}

	/*Nếu cho phép hiển thị wifi access point*/
	if (strlen(Config_Internet.Wifi_AP.Ssid) > 0 && Config_Internet.Wifi_AP.Enable == 0 && WifiAP_Auto_turnoff == false)
	{
		dbg_wifi("Mode AP STA access");
		WifiAP_is_run = true;
		WiFi.mode(WIFI_AP_STA);
		//Cau hinh AP
		WiFi.softAPConfig(Config_Internet.Wifi_AP.Ip, Config_Internet.Wifi_AP.Gw, Config_Internet.Wifi_AP.Sn);
		String Ssid_Ap = String(Config_Internet.Wifi_AP.Ssid) + "_" + String(Config_Device.Device.SerialID);
		// Ssid_Ap.toUpperCase();
		if (strlen(Config_Internet.Wifi_AP.Pass) >= 8)
			WiFi.softAP(Ssid_Ap.c_str(), Config_Internet.Wifi_AP.Pass);
		else
			WiFi.softAP(Ssid_Ap.c_str());
	}
	/*nếu không cho hiển thị acces point*/
	else
	{
		dbg_wifi("Mode STA acces");
		WifiAP_is_run = false;
		WiFi.mode(WIFI_STA);
	}

	/*connect tới wifi đích*/
	if (strlen(Config_Internet.Wifi_STA.Ssid) > 0)
	{
		dbg_wifi("connect to wifi: %s - %s", Config_Internet.Wifi_STA.Ssid, Config_Internet.Wifi_STA.Pass);
		String hostname = String(Config_Device.Device.HostName) + "_" + String(Config_Device.Device.SerialID);
		WiFi.setHostname(hostname.c_str()); //Tên đăng ký vào router

		/*Có enterprise*/
		if (Config_Internet.Enterprise.Enable == 1)
		{
			dbg_wifi("wifi enterprise user name :%s", Config_Internet.Enterprise.UserName);
			dbg_wifi("wifi enterprise ssid :%s", Config_Internet.Wifi_STA.Ssid);
			dbg_wifi("wifi enterprise password:%s", Config_Internet.Wifi_STA.Pass);
			WiFi.disconnect(true);
			// WiFi.mode(WIFI_STA);
			esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)Config_Internet.Enterprise.UserName, strlen(Config_Internet.Enterprise.UserName));
			esp_wifi_sta_wpa2_ent_set_username((uint8_t *)Config_Internet.Enterprise.UserName, strlen(Config_Internet.Enterprise.UserName));
			esp_wifi_sta_wpa2_ent_set_password((uint8_t *)Config_Internet.Wifi_STA.Pass, strlen(Config_Internet.Wifi_STA.Pass));
			esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
			esp_wifi_sta_wpa2_ent_enable(&config);
			WiFi.begin(Config_Internet.Wifi_STA.Ssid);
		}
		/*Không enterprise*/
		else
		{
			dbg_wifi("wifi nomal");
			if (strlen(Config_Internet.Wifi_STA.Pass) >= 8)
				WiFi.begin(Config_Internet.Wifi_STA.Ssid, Config_Internet.Wifi_STA.Pass);
			else
				WiFi.begin(Config_Internet.Wifi_STA.Ssid);
		}
	}
	/*Không có tên wifi để kết nối*/
	else
	{
		dbg_wifi("Dont connect to wifi");
		WiFi.disconnect();
	}
	/*đặt lại thời gian để thực hiện lại*/
	wifi_interval_reconnect.ToEUpdate(EW_WIFI_INTERVAL_RECONNECT);
}

void WifiHandle::ew_wifi_auto_process(ew_wifi_config_enum flag)
{
	/*nếu đang connect thì k cần quan tâm*/
	switch (WiFi.status())
	{
	case WL_CONNECTED:
		wifi_retry_connect = 0;
		return;
		// case WL_DISCONNECTED:
		// 	WiFi.disconnect();
		// 	break;
	}

	/*Nếu lần trước là đang cố găng connect wifi*/
	if (flag == ew_wifi_init || flag == ew_wifi_reinit)
	{
		if (wifi_interval_reconnect.ToEExpired() == true)
		{
			dbg_wifi("Timeout reconnect finish");
			/*retry giới hạn để reset moudle esp*/
			if (++wifi_retry_connect >= EW_WIFI_RETRY_NUM_RECONNECT)
			{
				wifi_retry_connect = 0;
				/*reset lại module esp*/
				ESPRebootTo.ToEUpdate(500);

				log_file_printf("Wifi Rebooting");
			}
			dbg_wifi("reconect for the %d", wifi_retry_connect);
			/*đặt lại thời gian để thực hiện lại*/
			wifi_interval_reconnect.ToEUpdate(EW_WIFI_INTERVAL_RECONNECT);
			ew_wifi_config_status(flag, 0);
		}
	}
}

void WifiHandle::ew_wifi_handle(void)
{
	static ew_wifi_config_enum wifi_status_old;
	switch (wifi_config_e)
	{
	case ew_wifi_init:
		/*chờ hết thời gian hoặc không có timeout nào*/
		if (wifi_config_to.ToEExpired() == false)
			return;
		dbg_wifi("Wifi mode init");
		/*config wifi*/
		ew_wifi_config();
		/*đã thực thi xong*/
		wifi_config_e = ew_wifi_finish;
		wifi_status_old = ew_wifi_init;
		break;
	case ew_wifi_reinit:
		/*chờ hết thời gian hoặc không có timeout nào*/
		if (wifi_config_to.ToEExpired() == false)
			return;
		dbg_wifi("Wifi mode reinit");
		/*de-config wifi*/
		// WiFi.disconnect();
		WiFi.mode(WIFI_OFF);
		WiFi.mode(WIFI_STA);
		/*config wifi*/
		ew_wifi_config();
		/*đã thực thi xong*/
		wifi_config_e = ew_wifi_finish;
		wifi_status_old = ew_wifi_reinit;
		break;
	case ew_wifi_deinit:
		/*chờ hết thời gian hoặc không có timeout nào*/
		if (wifi_config_to.ToEExpired() == false)
			return;
		dbg_wifi("Wifi mode deinit");
		/*de-config wifi*/
		WiFi.disconnect();
		WiFi.mode(WIFI_OFF);
		/*đã thực thi xong*/
		wifi_config_e = ew_wifi_finish;
		wifi_status_old = ew_wifi_deinit;
		break;
	case ew_wifi_finish:
		ew_wifi_auto_process(wifi_status_old);
		break;
	case ew_wifi_not_work:
		/*Không re-init, không làm gì khi được lệnh chờ*/
		break;
	}
}

WifiHandle Wifi_Handle;
