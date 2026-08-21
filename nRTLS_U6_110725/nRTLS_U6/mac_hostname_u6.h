#pragma once
#include <Arduino.h>
#include <ETH.h>
#include <ESPmDNS.h>
#include "NetBIOS.h"

extern "C" {
  #include "esp_system.h"
  #include "tcpip_adapter.h"   // ESP32 core 1.0.x
}

// ======= Cấu hình mặc định (có thể chỉnh trong .cpp) =======
extern const char* U6_HOSTNAME;          // tên thiết bị
extern bool        U6_MDNS_ENABLE_TELNET;
extern uint16_t    U6_MDNS_TELNET_PORT;

// ======= API =======

// 1) Đổi MAC ETH sớm (gọi TRƯỚC ETH.begin())
bool setCustomMACEarly(const uint8_t mac6[6]);  // truyền MAC vào
bool setCustomMACEarly();                       // dùng MAC mặc định ở .cpp

// 2) Đặt hostname cho giao diện ETH (core 1.0.x)
//    Giữ inline trong .h đúng như bạn yêu cầu
inline void setEthHostname(const char* hostname) {
  if (!hostname || !*hostname) return;
  tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_ETH, hostname);
}

// 3) In thông tin Ethernet (hostname, IP, mask, gateway, MAC)
void printEthInfo();

// 4) Khởi động mDNS (gọi khi ETH đã có IP)
void startMDNSOnce();

// 5) Khởi động NetBIOS/NBNS (gọi khi ETH đã có IP)
void startNBNSOnce(const char* hostname);

// 6) Tiện ích: start cả mDNS + NBNS + set hostname (an toàn gọi nhiều lần)
void U6NameSvc_startOnce();
