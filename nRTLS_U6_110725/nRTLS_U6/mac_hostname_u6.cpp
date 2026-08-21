#include "mac_hostname_u6.h"
#include "handle_config.h"

// ========= Cấu hình mặc định =========
const char *U6_HOSTNAME = "Ontrak-Hachi-U6";
bool U6_MDNS_ENABLE_TELNET = true;
uint16_t U6_MDNS_TELNET_PORT = 23;

// MAC mặc định (có thể sửa cho từng thiết bị)
// static const uint8_t U6_DEFAULT_MAC[6] = {0x17, 0x52, 0x65, 0x69, 0x88,
// 0x52}; static const uint8_t U6_DEFAULT_MAC[6] = {0x8C, 0x1F, 0x64, 0xCE,
// 0xD0, 0x05};
static const uint8_t U6_DEFAULT_MAC[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// ========= State nội bộ =========
static bool mdnsStarted = false;
static bool nbnsStarted = false;

// ========= 1) Đổi MAC ETH sớm =========
static inline void make_unicast_laa(uint8_t &mac0) {
  mac0 &= ~0x01; // bit0=0 -> unicast
  mac0 |= 0x02;  // bit1=1 -> locally administered
}

// --- Hàm trừ 3 cho địa chỉ MAC (48 bit) ---
// static void mac_minus_3(const uint8_t in[6], uint8_t out[6])
// {
//   int borrow = 3;
//   for (int i = 5; i >= 0; --i)
//   {
//     int val = (int)in[i] - (borrow & 0xFF);
//     if (val < 0)
//     {
//       val += 256;
//       borrow = 1;
//     }
//     else
//     {
//       borrow = 0;
//     }
//     out[i] = (uint8_t)val;
//   }
// }
// static void mac_minus_3(const uint8_t in[6], uint8_t out[6])
// {
//   int borrow = 3; // Giá trị cần trừ ban đầu
//   for (int i = 5; i >= 0; --i)
//   {
//     int16_t val = (int16_t)in[i] - borrow;
//     if (val < 0)
//     {
//       val += 256;
//       borrow = 1; // mượn 1 ở byte kế tiếp
//     }
//     else
//     {
//       borrow = 0; // không cần mượn nữa
//     }
//     out[i] = (uint8_t)val;
//   }
// }
// --- Hàm trừ 3 cho địa chỉ MAC (theo quy tắc ESP32: chỉ tác động lên byte
// cuối) ---
static void mac_minus_3(const uint8_t in[6], uint8_t out[6]) {
  // Copy nguyên MAC đầu vào
  memcpy(out, in, 6);

  // Trừ 3 ở byte cuối cùng, modulo 256 (vì ESP32 chỉ cộng/trừ trên byte cuối)
  out[5] = (uint8_t)(in[5] - 3);
}

// --- Hàm chính đặt MAC cho ESP32 trước ETH.begin() ---
bool setCustomMACEarly(const uint8_t mac6[6]) {
  if (!mac6)
    return false;

  // Tính base MAC = eth_mac - 3 (ESP32 dùng base +3 cho Ethernet)
  uint8_t base[6];
  mac_minus_3(mac6, base);

  // Đảm bảo unicast (bit0 = 0)
  base[0] &= 0xFE;

  // Áp dụng base MAC
  esp_err_t err = esp_base_mac_addr_set(base);

  if (err == ESP_OK) {
    dbg_enthernet("\nCustom base MAC set OK -> EMAC dự kiến: "
                  "%02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac6[0], mac6[1], mac6[2], mac6[3], mac6[4], mac6[5]);
  } else {
    dbg_enthernet("ESP MAC set FAIL! code=%d\n", err);
  }

  return (err == ESP_OK);
}

bool setCustomMACEarly() { return setCustomMACEarly(U6_DEFAULT_MAC); }

// ========= 3) In thông tin ETH =========
void printEthInfo() {
  dbg_enthernet("========== ETH INFO ==========");
  dbg_enthernet("Hostname: %s\n", U6_HOSTNAME);
  dbg_enthernet("IP:       %s\n", ETH.localIP().toString().c_str());
  dbg_enthernet("Mask:     %s\n", ETH.subnetMask().toString().c_str());
  dbg_enthernet("Gateway:  %s\n", ETH.gatewayIP().toString().c_str());

  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_ETH);
  dbg_enthernet("MAC:      %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1],
                mac[2], mac[3], mac[4], mac[5]);
  dbg_enthernet("==============================");
}

// ========= 4) mDNS =========
void startMDNSOnce() {
  if (mdnsStarted)
    return;

  if (!MDNS.begin(U6_HOSTNAME)) {
    dbg_enthernet("[mDNS] Failed to start mDNS responder");
    return;
  }
}

// ========= 5) NBNS/NetBIOS =========
void startNBNSOnce(const char *hostname) {
  if (nbnsStarted)
    return;
  if (!hostname || !*hostname)
    return;

  String nb = String(hostname);
  nb.toUpperCase();
  if (nb.length() > 15)
    nb = nb.substring(0, 15); // NetBIOS <= 15 ký tự

  if (NBNS.begin(nb.c_str())) {
    nbnsStarted = true;
    dbg_enthernet("[NBNS] Started: %s\n", nb.c_str());
  } else {
    dbg_enthernet("[NBNS] Failed to start");
  }
}

// ========= 6) Gộp: hostname + mDNS + NBNS =========
// void U6NameSvc_startOnce()
// {
//   if (!ETH.linkUp() || ETH.localIP() == INADDR_NONE)
//     return;

//   // đặt hostname (inline trong .h, core 1.0.x)
//   setEthHostname(U6_HOSTNAME);

//   // dịch vụ tên
//   startMDNSOnce();
//   startNBNSOnce(U6_HOSTNAME);
// }
void U6NameSvc_startOnce() {
  if (!ETH.linkUp() || ETH.localIP() == INADDR_NONE)
    return;

  // 1) Lấy hostname hiện thời từ TCP/IP stack
  const char *cur = nullptr;
  if (tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_ETH, &cur) != 0 || !cur ||
      !cur[0]) {
    // fallback: ưu tiên hostname đã lưu trong Config_Device, rồi đến default
    cur = (Config_Device.Device.HostName[0]) ? Config_Device.Device.HostName
                                             : U6_HOSTNAME;
    setEthHostname(cur); // đặt lại để đảm bảo đồng bộ
  }

  // 2) mDNS: khởi động 1 lần với tên hiện thời
  static bool mdns_started = false;
  if (!mdns_started) {
    if (MDNS.begin(cur)) {
      mdns_started = true;
      if (U6_MDNS_ENABLE_TELNET) {
        MDNS.addService("telnet", "tcp", U6_MDNS_TELNET_PORT);
      }
      dbg_enthernet("\n[mDNS] started as %s.local\n", cur);
    } else {
      dbg_enthernet("\n[mDNS] begin failed");
    }
  }

  // 3) NBNS với tên hiện thời
  startNBNSOnce(cur);
}
