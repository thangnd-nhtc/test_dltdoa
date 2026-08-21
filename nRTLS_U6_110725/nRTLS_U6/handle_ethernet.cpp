#include "handle_ethernet.h"
#include "handle_com_regs.h"
#include "handle_config.h"
#include "handle_spi_master.h"
#include "handle_wifi.h"
#include <ETH.h>


volatile bool eth_connected = false;
volatile bool wifi_connected = false;

uint8_t retry_conncet = 0;
TimeOutEvent eth_Wait_error(0);
TimeOutEvent Eth_TimeCheck(0);
TimeOutEvent Wifi_TimeCheck(0);

handleEthernet::handleEthernet(/* args */) {}

handleEthernet::~handleEthernet() {}

// Set your Static IP address
IPAddress local_IP(192, 168, 21, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 21, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional

void WiFiEvent(WiFiEvent_t event) {
  String hostname = String(Config_Device.Device.HostName) + "_" +
                    String(Config_Device.Device.SerialID);
  switch (event) {
    /*Wifi Mode STA*/
  case SYSTEM_EVENT_STA_START: {
    dbg_enthernet("Wifi STA START");
    WiFi.setAutoReconnect(true);

    /*nếu cho phép ip động*/
    // if (Config_Internet.DHCP_setting.DHCPEn == 1)
    // {
    // 	WiFi.dnsIP(0);
    // 	dbg_enthernet("Ethernet DHCP Enable");
    // }
    // else
    // {
    // 	dbg_enthernet("Ethernet DHCP Disable");
    // 	WiFi.dnsIP(1);
    // 	IPAddress E_IP,E_GW,E_SN,E_DNS;
    // 	E_IP.fromString(String(Config_Internet.DHCP_setting.Ip));
    // 	E_GW.fromString(String(Config_Internet.DHCP_setting.Gw));
    // 	E_SN.fromString(String(Config_Internet.DHCP_setting.Sn));
    // 	E_DNS.fromString(String(Config_Internet.DHCP_setting.Dns));
    // 	WiFi.config(E_IP,E_GW,E_SN,E_DNS);
    // }

    Eth_TimeCheck.ToEUpdate(20000);
    break;
  }

  case SYSTEM_EVENT_STA_CONNECTED: /**< ESP32 station connected to AP */
  {
    dbg_enthernet("Wifi STA CONNECTED");
    WiFi.enableIpV6(); // enable sta ipv6 here
    Wifi_TimeCheck.ToEUpdate(10000);
    break;
  }

  case SYSTEM_EVENT_STA_DISCONNECTED: /**< ESP32 station disconnected from AP */
    dbg_enthernet("Wifi STA DISCONNECTED");
    wifi_connected = false;
    FlagcheckLed = true;
    // PTB ẩn
    // if (!eth_connected)
    // 	WiFi.reconnect();

    // retry_conncet++;
    // if(retry_conncet > 5)
    // {
    // 	retry_conncet = 0;
    // 	Eth_TimeCheck.ToEUpdate(1);
    // 	Wifi_TimeCheck.ToEDisable();
    // }

    break;
  // case SYSTEM_EVENT_STA_AUTHMODE_CHANGE: /**< the auth mode of AP connected
  // by ESP32 station changed */ 	dbg_enthernet("Wifi STA AUTHMODE CHANGE");
  // 	break;
  case SYSTEM_EVENT_STA_GOT_IP: /**< ESP32 station got IP from connected AP */

    wifi_connected = true;
    Eth_TimeCheck.ToEDisable();

    strncpy(Config_Internet.Ethernet.Mac, ETH.macAddress().c_str(), 32);
    strncpy(Config_Internet.Wifi_STA.Mac, WiFi.macAddress().c_str(), 32);
    Config_Internet.Wifi_STA.Ip = WiFi.localIP();
    Config_Internet.Wifi_STA.Gw = WiFi.gatewayIP();
    Config_Internet.Wifi_STA.Sn = WiFi.subnetMask();
    Config_Internet.Wifi_STA.Dns = WiFi.dnsIP();

    dbg_enthernet("WMAC: %s", Config_Internet.Wifi_STA.Mac);
    dbg_enthernet("WIp: %s", Config_Internet.Wifi_STA.Ip.toString().c_str());
    dbg_enthernet("WGw: %s", Config_Internet.Wifi_STA.Gw.toString().c_str());
    dbg_enthernet("WSn: %s", Config_Internet.Wifi_STA.Sn.toString().c_str());
    dbg_enthernet("WDns: %s", Config_Internet.Wifi_STA.Dns.toString().c_str());

    FlagcheckLed = true;
    break;

  case SYSTEM_EVENT_AP_STACONNECTED: /**< a station connected to ESP32 soft-AP
                                      */
    dbg_enthernet("Wifi AP STA CONNECTED");
    Wifi_Handle.ew_wifi_config_status(ew_wifi_not_work, 0);
    break;

  case SYSTEM_EVENT_AP_STADISCONNECTED: /**< a station disconnected from ESP32
                                           soft-AP */
    dbg_enthernet("Wifi AP STA DISCONNECTED");
    Wifi_Handle.ew_wifi_config_status(ew_wifi_finish, 0);
    break;
    // case SYSTEM_EVENT_AP_STAIPASSIGNED: /**< ESP32 soft-AP assign an IP to a
    // connected station */ 	dbg_enthernet("Wifi AP STA IP ASSIGNED"); 	break;

    /*Mode Ethernet*/
  case SYSTEM_EVENT_ETH_START:
    dbg_enthernet("ETH Started");
    /*nếu cho phép ip động*/
    if (Config_Internet.DHCP_setting.DHCPEn == 1) {
      ETH.dnsIP(0);
      dbg_enthernet("Ethernet DHCP Enable");
      // in MAC Ethernet
      uint8_t mac[6];
      esp_read_mac(mac, ESP_MAC_ETH);
      dbg_enthernet("\nEthernet MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],
                    mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
      dbg_enthernet("Ethernet DHCP Disable");
      ETH.dnsIP(1);
      IPAddress E_IP, E_GW, E_SN, E_DNS;
      E_IP.fromString(String(Config_Internet.DHCP_setting.Ip));
      E_GW.fromString(String(Config_Internet.DHCP_setting.Gw));
      E_SN.fromString(String(Config_Internet.DHCP_setting.Sn));
      E_DNS.fromString(String(Config_Internet.DHCP_setting.Dns));
      ETH.config(E_IP, E_GW, E_SN, E_DNS);
    }

    // set eth hostname here
    // ETH.setHostname(hostname.c_str());//Hieu
    // ETH.setHostname("ONTRAK HACHI");//Hieu
    /*báo busy*/
    // eth_Wait_error.ToEUpdate(20000);
    Eth_TimeCheck.ToEUpdate(10000);
    break;
  case SYSTEM_EVENT_ETH_CONNECTED: {
    dbg_enthernet("ETH Connected");
    // in MAC Ethernet
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_ETH);
    dbg_enthernet("\nEthernet MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],
                  mac[1], mac[2], mac[3], mac[4], mac[5]);

    Eth_TimeCheck.ToEUpdate(30000);
    break;
  }

  case SYSTEM_EVENT_ETH_GOT_IP: {
    eth_connected = true;
    Eth_TimeCheck.ToEDisable();
    eth_Wait_error.ToEDisable();
    // eth_Wait_error.ToEUpdate(90000); // Sau 90s không kết nối tới MQTT thì
    // kết nối wifi

    strncpy(Config_Internet.Ethernet.Mac, ETH.macAddress().c_str(), 32);
    strncpy(Config_Internet.Wifi_STA.Mac, WiFi.macAddress().c_str(), 32);
    Config_Internet.Ethernet.Ip = ETH.localIP();
    Config_Internet.Ethernet.Gw = ETH.gatewayIP();
    Config_Internet.Ethernet.Sn = ETH.subnetMask();
    Config_Internet.Ethernet.Dns = ETH.dnsIP();

    dbg_enthernet("EMAC: %s", Config_Internet.Ethernet.Mac);
    dbg_enthernet("EIp: %s", Config_Internet.Ethernet.Ip.toString().c_str());
    dbg_enthernet("EGw: %s", Config_Internet.Ethernet.Gw.toString().c_str());
    dbg_enthernet("ESn: %s", Config_Internet.Ethernet.Sn.toString().c_str());
    dbg_enthernet("EDns: %s", Config_Internet.Ethernet.Dns.toString().c_str());

    // Serial.printf("EMAC: %s \n", Config_Internet.Ethernet.Mac);
    // Serial.printf("EIp: %s \n",
    // Config_Internet.Ethernet.Ip.toString().c_str()); Serial.printf("EGw: %s
    // \n", Config_Internet.Ethernet.Gw.toString().c_str()); Serial.printf("ESn:
    // %s \n", Config_Internet.Ethernet.Sn.toString().c_str());
    // Serial.printf("EDns: %s \n",
    // Config_Internet.Ethernet.Dns.toString().c_str());

    FlagcheckLed = true;
    // if (wifi_connected == true)
    // 	Wifi_Handle.ew_wifi_config_status(ew_wifi_deinit, 0);
    break;
  }

    // case SYSTEM_EVENT_ETH_GOT_IP:
    // {
    // 	eth_connected = true;
    // 	Eth_TimeCheck.ToEDisable();
    // 	eth_Wait_error.ToEDisable();
    // 	// eth_Wait_error.ToEUpdate(90000); // Sau 90s không kết nối tới MQTT
    // thì kết nối wifi

    // 	// ĐỌC GIÁ TRỊ HIỆN TẠI
    // 	strncpy(Config_Internet.Ethernet.Mac, ETH.macAddress().c_str(), 32);
    // 	strncpy(Config_Internet.Wifi_STA.Mac, WiFi.macAddress().c_str(), 32);
    // 	Config_Internet.Ethernet.Ip = ETH.localIP();
    // 	Config_Internet.Ethernet.Gw = ETH.gatewayIP();
    // 	Config_Internet.Ethernet.Sn = ETH.subnetMask();
    // 	Config_Internet.Ethernet.Dns = ETH.dnsIP();

    // 	// ===== Nếu thiếu DNS → tự đặt =====
    // 	if (Config_Internet.Ethernet.Dns == IPAddress(0, 0, 0, 0))
    // 	{
    // 		IPAddress ip = Config_Internet.Ethernet.Ip;
    // 		IPAddress gw = Config_Internet.Ethernet.Gw;
    // 		IPAddress sn = Config_Internet.Ethernet.Sn;
    // 		IPAddress dns;

    // 		if (gw != IPAddress(0, 0, 0, 0))
    // 		{
    // 			// Ưu tiên dùng gateway làm DNS (router thường chạy DNS
    // proxy) 			dns = gw;
    // 		}
    // 		else
    // 		{
    // 			// Tự suy ra <network>.1 từ IP được cấp (vd 192.168.0.11
    // -> 192.168.0.1) 			dns = IPAddress(ip[0], ip[1], ip[2], 1);
    // 			// Đồng thời gán luôn gateway nếu chưa có
    // 			gw = dns;
    // 		}

    // 		// Gán lại cấu hình, giữ nguyên IP/Subnet, bổ sung DNS (và GW
    // nếu trống) 		ETH.config(ip, gw, sn, dns);

    // 		// Cập nhật lại biến lưu sau khi config
    // 		Config_Internet.Ethernet.Gw = gw;
    // 		Config_Internet.Ethernet.Dns = dns;
    // 	}

    // 	dbg_enthernet("EMAC: %s", Config_Internet.Ethernet.Mac);
    // 	dbg_enthernet("EIp: %s",
    // Config_Internet.Ethernet.Ip.toString().c_str()); 	dbg_enthernet("EGw: %s",
    // Config_Internet.Ethernet.Gw.toString().c_str()); 	dbg_enthernet("ESn: %s",
    // Config_Internet.Ethernet.Sn.toString().c_str()); 	dbg_enthernet("EDns:
    // %s", Config_Internet.Ethernet.Dns.toString().c_str());

    // 	Serial.printf("EMAC: %s \n", Config_Internet.Ethernet.Mac);
    // 	Serial.printf("EIp: %s \n",
    // Config_Internet.Ethernet.Ip.toString().c_str()); 	Serial.printf("EGw: %s
    // \n", Config_Internet.Ethernet.Gw.toString().c_str()); 	Serial.printf("ESn:
    // %s \n", Config_Internet.Ethernet.Sn.toString().c_str());
    // 	Serial.printf("EDns: %s \n",
    // Config_Internet.Ethernet.Dns.toString().c_str());

    // 	FlagcheckLed = true;
    // 	// if (wifi_connected == true)
    // 	//     Wifi_Handle.ew_wifi_config_status(ew_wifi_deinit, 0);
    // 	break;
    // }

  case SYSTEM_EVENT_ETH_DISCONNECTED: {
    dbg_enthernet("ETH Disconnected");
    eth_connected = false;
    FlagcheckLed = true;
    Eth_TimeCheck.ToEUpdate(100);
    // eth_Wait_error.ToEUpdate(20000);
    // if (wifi_connected == false)
    // 	Wifi_Handle.ew_wifi_config_status(ew_wifi_reinit, 0);
    break;
  }

  // case SYSTEM_EVENT_ETH_STOP:
  // 	Wifi_Handle.ew_wifi_config_status(ew_wifi_reinit, 0);
  // 	dbg_enthernet("ETH Stopped");
  // 	eth_connected = false;
  // 	break;
  default:
    break;
  }
}

// void reset_phy_clock()
// {
// 	pinMode(ETH_CLK_EN, OUTPUT);
// 	digitalWrite(ETH_CLK_EN, LOW);
// 	delay(50); // tắt oscillator 50ms
// 	digitalWrite(ETH_CLK_EN, HIGH);
// 	delay(200); // chờ clock ổn định
// }
void reset_phy_clock() {
  pinMode(ETH_CLK_EN, OUTPUT);
  // digitalWrite(ETH_CLK_EN, HIGH);
  // delay(50); // tắt oscillator 50ms
  digitalWrite(ETH_CLK_EN, LOW);
  delay(200); // chờ clock ổn định
}

void handleEthernet::eth_setup() {
  WiFi.onEvent(WiFiEvent);
  // pinMode(5, OUTPUT);
  // digitalWrite(5, HIGH);
  // delay(1000);
  reset_phy_clock();
  WiFi.mode(WIFI_OFF);
  // ETH.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE,
            DEFINE_ETH_CLK_MODE);
  // in MAC Ethernet
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_ETH);
  dbg_enthernet("\nEthernet MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],
                mac[1], mac[2], mac[3], mac[4], mac[5]);
  eth_Wait_error.ToEUpdate(90000);
}

bool handleEthernet::lan_isconnect() { return eth_connected; }
bool handleEthernet::wifi_isconnect() { return wifi_connected; }

void handleEthernet::eth_loop() {
  // static bool led_wifi_handle_f = true;
  // static bool led_ethernet_handle_f = true;
  // if (eth_connected == true)
  // {
  // 	// if (led_wifi_handle_f == true)
  // 	// 	LED_STT.attach_ms(99, 200, 900);
  // 	led_wifi_handle_f = false;
  // 	led_ethernet_handle_f = true;
  // }
  // else if (wifi_connected == true)
  // {
  // 	// if (led_ethernet_handle_f == true)
  // 	// 	LED_STT.attach_ms(99, 80, 420);
  // 	led_wifi_handle_f = true;
  // 	led_ethernet_handle_f = false;
  // }
  // else
  // {
  // 	led_wifi_handle_f = true;
  // 	led_ethernet_handle_f = true;
  // 	// LED_STT.attach_ms(111, 20, 980);
  // }

  /*Chờ ổn định mới check lại giao tiếp*/
  // if (eth_Wait_error.ToEExpired() == true && eth_connected == false)
  // {
  // 	/*Nếu không kết nối được internet thì bắt đầu kết nối wifi*/
  // 	// if (eth_connected == false)
  // 	// {
  // 		dbg_enthernet("Ethernet connect fail continuos connect wifi");
  // 		eth_Wait_error.ToEDisable();
  // 		Eth_TimeCheck.ToEUpdate(90000);
  // 		Wifi_TimeCheck.ToEDisable();
  // 		dbg_enthernet("Wifi AP");
  // 		//String namewifi = "ONTRAK HACHI_"; //
  // +Config_Device.Device.SerialID+" 		String namewifi =
  // Config_Internet.Wifi_AP.Ssid; 		String serialID =
  // String(Config_Device.Device.SerialID); 		namewifi += "_"; 		namewifi +=
  // serialID; 		dbg_enthernet("name wifi: %s",namewifi);
  // 		// strcat(namewifi,);
  // 		WiFi.softAP(namewifi.c_str(), Config_Internet.Wifi_AP.Pass);
  // 		IPAddress IP = WiFi.softAPIP();
  // 		dbg_enthernet("AP IP address: ");
  // 		Serial.println(IP);
  // 		// Wifi_TimeCheck.ToEUpdate(1);
  // 		// Wifi_Handle.ew_wifi_config_status(ew_wifi_init, 0);
  // 	// }
  // 	/*nếu có IP từ LAN mà không lấy được token key*/
  // 	// else if (cmd13_is_tokenkey() == false)
  // 	// {
  // 	// 	dbg_enthernet("Lost token key");
  // 	// 	Wifi_Handle.ew_wifi_config_status(ew_wifi_init, 0);
  // 	// }
  // }

  /*Nếu đã có key thì tắt wifi AP*/
  // if (Wifi_Handle.ew_wifiAP_is_run() == true /*&& cmd13_is_tokenkey() ==
  // true*/)
  // {
  // 	dbg_enthernet("Turn off wifi AP");
  // 	Wifi_Handle.ew_wifiAP_turnOff();
  // 	Wifi_Handle.ew_wifi_config_status(ew_wifi_reinit, 0);
  // }

  /*handle wifi*/
  // Wifi_Handle.ew_wifi_handle();

  /*Nếu báo connectted mà trong 10s k có ip thì cấu hình lại*/
  if (Eth_TimeCheck.ToEExpired()) {
    // dbg_enthernet("De-init ethernet");
    // ETH.de_init();
    dbg_enthernet("Re-init ethernet");
    // in địa chỉ MAC
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_ETH);
    dbg_enthernet("\nEthernet MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],
                  mac[1], mac[2], mac[3], mac[4], mac[5]);
    reset_phy_clock(); // PTB
    WiFi.mode(WIFI_OFF);
    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE,
              ETH_CLK_MODE);
    // eth_Wait_error.ToEUpdate(20000);
  }
  // if (Wifi_TimeCheck.ToEExpired())
  // {
  // 	// WiFi.onEvent(WiFiEvent);
  // 	WiFi.mode(WIFI_STA);
  // 	WiFi.begin(Config_Internet.Wifi_STA.Ssid,
  // Config_Internet.Wifi_STA.Pass);
  // 	// if (strlen(Config_Internet.Wifi_STA.Pass) >= 8)
  // 	// 	WiFi.begin(Config_Internet.Wifi_STA.Ssid,
  // Config_Internet.Wifi_STA.Pass);
  // 	// else
  // 	// 	WiFi.begin(Config_Internet.Wifi_STA.Ssid);
  // }
}

handleEthernet handle_ethernet;
