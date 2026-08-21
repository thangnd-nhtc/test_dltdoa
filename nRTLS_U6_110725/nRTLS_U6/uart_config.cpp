#include "uart_config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "handle_config.h" // extern Config_Device, Config_Internet, BASECONFIG
#include <esp_system.h>    // esp_base_mac_addr_set, esp_read_mac

#include "mac_hostname_u6.h"
#include "handle_ethernet.h"
#include "handle_wifi.h"

// ====== extern từ handle_config.h ======
extern ConfigDevice Config_Device;
extern ConfigInternet Config_Internet;

void saveDevIdRaw(uint32_t id)
{
    // ensureEEPROM();
    EEPROM.put(EEPROM_ADDR_DEVID, id);
    EEPROM.commit();
}
// Gói tiện ích: push + (tùy chọn) confirm
bool pushAndConfirmPeerDeviceId(uint32_t id, bool doConfirm)
{
    bool okPush = pushPeerDeviceId(id);
    if (!okPush)
        return false;
    if (!doConfirm)
        return true;

    uint32_t back = 0;
    if (queryPeerDeviceId(back) && back == id)
        return true;
    //Serial.println("[CFG] WARN: cannot confirm DEVID? from B");
    return false; // push OK nhưng confirm fail
}

// === Helpers gửi lệnh sang ESP32-B qua Serial1 ===
static bool readLineSerial1(String &out, uint32_t timeout_ms = 800)
{
    Serial1.setTimeout(timeout_ms);
    out = Serial1.readStringUntil('\n'); // không chứa '\n'
    while (out.endsWith("\r"))
        out.remove(out.length() - 1);
    return out.length() > 0;
}

// Gửi "DEVID=<id>\r\n" và chờ "OK DEVID=<...>" static
bool pushPeerDeviceId(uint32_t id,
                             uint8_t maxRetries,
                             uint32_t respTimeoutMs)
{
    // xóa rác còn trong RX
    while (Serial1.available())
        Serial1.read();

    for (uint8_t attempt = 1; attempt <= maxRetries; ++attempt)
    {
        //Serial.printf("[CFG] Push DEVID to B (try %u) ...\n", attempt);
        Serial1.printf("DEVID=%lu\r\n", (unsigned long)id);

        uint32_t t0 = millis();
        while (millis() - t0 < respTimeoutMs)
        {
            if (!Serial1.available())
            {
                delay(1);
                continue;
            }
            String line;
            if (!readLineSerial1(line, respTimeoutMs))
                continue;

            if (line.startsWith("OK ") && line.indexOf("DEVID=") >= 0)
            {
                //Serial.printf("[CFG] B replied: %s\n", line.c_str());
                return true;
            }
            if (line.startsWith("ERR"))
            {
                //Serial.printf("[CFG] B error: %s\n", line.c_str());
                break; // thử lại
            }
        }
    }
    return false;
}

// (tuỳ chọn) Hỏi lại B để xác nhận Static
bool queryPeerDeviceId(uint32_t &outId,
                              uint8_t maxLines,
                              uint32_t respTimeoutMs)
{
    while (Serial1.available())
        Serial1.read();
    Serial1.print("DEVID?\r\n");

    for (uint8_t i = 0; i < maxLines; ++i)
    {
        String line;
        if (!readLineSerial1(line, respTimeoutMs))
            continue;

        int p = line.indexOf("DEVID=");
        if (line.startsWith("OK ") && p >= 0)
        {
            const char *s = line.c_str() + p + 6;
            char *endp = nullptr;
            uint32_t val = (strncmp(s, "0x", 2) == 0 || strncmp(s, "0X", 2) == 0)
                               ? strtoul(s, &endp, 16)
                               : strtoul(s, &endp, 10);
            if (endp && *endp == '\0')
            {
                outId = val;
                return true;
            }
        }
    }
    return false;
}

// // Gói tiện ích: push + (tùy chọn) confirm
// static bool pushAndConfirmPeerDeviceId(uint32_t id, bool doConfirm = true)
// {
//     bool okPush = pushPeerDeviceId(id);
//     if (!okPush)
//         return false;
//     if (!doConfirm)
//         return true;

//     uint32_t back = 0;
//     if (queryPeerDeviceId(back) && back == id)
//         return true;
//     Serial.println("[CFG] WARN: cannot confirm DEVID? from B");
//     return false; // push OK nhưng confirm fail
// }

// ==== EEPROM layout khớp với bạn đã dùng ====
// MAC: 6 bytes @0
// HOST: 32 bytes @6
// DEBUG flag: @EEPROM_ADDR_DEBUG (đã có)
bool eepromReadMac(uint8_t out[6])
{
    bool all0 = true, allf = true;
    for (int i = 0; i < 6; i++)
    {
        out[i] = EEPROM.read(0 + i);
        if (out[i] != 0x00)
            all0 = false;
        if (out[i] != 0xFF)
            allf = false;
    }
    // hợp lệ: không toàn 0x00/0xFF và là unicast (bit0=0)
    if (all0 || allf)
        return false;
    if (out[0] & 0x01)
        return false; // multicast -> không dùng
    return true;
}

bool eepromReadHostname(String &host)
{
    char buf[32]; // đúng 32 byte vùng host
    for (int i = 0; i < 32; i++)
        buf[i] = EEPROM.read(6 + i);
    buf[31] = 0;
    // hợp lệ: không rỗng, độ dài <= 31, chỉ [a-z0-9-], không bắt/đặt dấu '-'
    auto validHostname = [](const char *h) -> bool
    {
        size_t n = strlen(h);
        if (n == 0 || n > 31)
            return false;
        if (h[0] == '-' || h[n - 1] == '-')
            return false;
        for (size_t i = 0; i < n; i++)
        {
            char c = tolower(h[i]);
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'))
                return false;
        }
        return true;
    };
    if (!validHostname(buf))
        return false;
    host = String(buf);
    return true;
}
bool eepromReadDeviceID(uint32_t &outId)
{
    // Đọc 4 byte từ vùng EEPROM_ADDR_DEVID
    uint32_t id = 0xFFFFFFFF;
    EEPROM.get(EEPROM_ADDR_DEVID, id);

    // Kiểm tra hợp lệ: khác 0 và khác 0xFFFFFFFF
    if (id == 0 || id == 0xFFFFFFFF)
        return false;

    outId = id;
    return true;
}

// ====== nội bộ ======
namespace UartConfig
{

    static bool s_eepromInited = false;
    static EthReinitHook s_ethReinitHook = nullptr;

    static inline void ensureEEPROM()
    {
        if (!s_eepromInited)
        {
            EEPROM.begin(EEPROM_SIZE);
            s_eepromInited = true;
        }
    }

    // --- helpers ---
    bool parseMac(const char *s, uint8_t mac[6])
    {
        if (!s)
            return false;
        int b[6];
        if (sscanf(s, "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6)
            return false;
        for (int i = 0; i < 6; i++)
            mac[i] = (uint8_t)b[i];
        // phải là unicast (bit0 của octet đầu = 0)
        if (mac[0] & 0x01)
            return false;
        return true;
    }

    bool validHostname(const char *host)
    {
        if (!host)
            return false;
        size_t n = strlen(host);
        if (n == 0 || n > 31)
            return false; // giới hạn MDNS/DHCP-friendly
        if (host[0] == '-' || host[n - 1] == '-')
            return false;
        for (size_t i = 0; i < n; i++)
        {
            char c = tolower(host[i]);
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'))
                return false;
        }
        return true;
    }

    static void macToStr(const uint8_t m[6], char out[20])
    {
        snprintf(out, 20, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
    }

    static bool loadMacRaw(uint8_t mac[6])
    {
        ensureEEPROM();
        for (int i = 0; i < 6; i++)
            mac[i] = EEPROM.read(EEPROM_ADDR_MAC + i);
        bool all0 = true, allf = true;
        for (int i = 0; i < 6; i++)
        {
            if (mac[i] != 0)
                all0 = false;
            if (mac[i] != 0xFF)
                allf = false;
        }
        return !(all0 || allf);
    }

    static void saveMacRaw(const uint8_t mac[6])
    {
        ensureEEPROM();
        for (int i = 0; i < 6; i++)
            EEPROM.write(EEPROM_ADDR_MAC + i, mac[i]);
        EEPROM.commit();
    }

    static void loadHostRaw(char out[32])
    {
        ensureEEPROM();
        for (int i = 0; i < 32; i++)
            out[i] = EEPROM.read(EEPROM_ADDR_HOST + i);
        out[31] = 0;
    }
    static void saveHostRaw(const char *host)
    {
        ensureEEPROM();
        size_t n = strlen(host);
        for (int i = 0; i < 32; i++)
        {
            char c = (i < (int)n) ? host[i] : 0;
            EEPROM.write(EEPROM_ADDR_HOST + i, c);
        }
        EEPROM.commit();
    }

    static uint32_t loadDevIdRaw()
    {
        ensureEEPROM();
        uint32_t id = 0xFFFFFFFF;
        EEPROM.get(EEPROM_ADDR_DEVID, id);
        return id;
    }
    // static void saveDevIdRaw(uint32_t id)
    // {
    //     ensureEEPROM();
    //     EEPROM.put(EEPROM_ADDR_DEVID, id);
    //     EEPROM.commit();
    // }

    // ====== MIRROR ra baseconfig.txt (atomic) ======
    static bool beginFS()
    {
        static bool inited = false;
        if (!inited)
            inited = SPIFFS.begin(true);
        return inited;
    }

    static void jsonMergeKey(JsonDocument &doc, const char *k, const char *v)
    {
        if (v && *v)
            doc[k] = v;
        else
            doc[k] = "";
    }
    static void jsonMergeKey(JsonDocument &doc, const char *k, uint32_t v)
    {
        doc[k] = v;
    }

    // static bool saveMirrorToBaseConfig(const uint8_t *mac /*nullable*/, const char *host /*nullable*/, const uint32_t *devid /*nullable*/)
    // {
    //     if (!beginFS())
    //         return false;

    //     // Đọc file hiện tại (nếu có)
    //     DynamicJsonDocument doc(4096);
    //     {
    //         File fr = SPIFFS.open(BASECONFIG, "r");
    //         if (fr)
    //         {
    //             DeserializationError er = deserializeJson(doc, fr);
    //             fr.close();
    //             if (er)
    //             {
    //                 // nếu lỗi parse -> reset doc rỗng
    //                 doc.clear();
    //             }
    //         }
    //     }

    //     // Gộp 3 field quan trọng
    //     if (mac)
    //     {
    //         char macs[20] = {0};
    //         macToStr(mac, macs);
    //         doc["EMAC"] = macs; // dùng khóa EMAC trong file của bạn
    //     }
    //     if (host)
    //     {
    //         doc["HostName"] = host;
    //     }
    //     if (devid)
    //     {
    //         doc["SerialID"] = *devid;
    //     }

    //     // Atomic write: .tmp rồi rename
    //     const char *tmpPath = "/baseconfig.tmp";
    //     File fw = SPIFFS.open(tmpPath, "w");
    //     if (!fw)
    //         return false;
    //     bool ok = (serializeJsonPretty(doc, fw) > 0);
    //     fw.flush();
    //     fw.close();
    //     if (!ok)
    //     {
    //         SPIFFS.remove(tmpPath);
    //         return false;
    //     }

    //     // Ghi xong -> thay thế
    //     SPIFFS.remove(BASECONFIG);
    //     if (!SPIFFS.rename(tmpPath, BASECONFIG))
    //     {
    //         // fallback: copy
    //         File fr2 = SPIFFS.open(tmpPath, "r");
    //         File fw2 = SPIFFS.open(BASECONFIG, "w");
    //         if (fr2 && fw2)
    //         {
    //             while (fr2.available())
    //                 fw2.write(fr2.read());
    //             fw2.flush();
    //             fw2.close();
    //             fr2.close();
    //             SPIFFS.remove(tmpPath);
    //             return true;
    //         }
    //         if (fr2)
    //             fr2.close();
    //         if (fw2)
    //             fw2.close();
    //         SPIFFS.remove(tmpPath);
    //         return false;
    //     }
    //     return true;
    // }
    static bool isMacString6Octets(const char *s)
    {
        if (!s)
            return false;
        // Định dạng "AA:BB:CC:DD:EE:FF" => 17 ký tự
        if (strlen(s) != 17)
            return false;
        for (int i = 0; i < 17; ++i)
        {
            if ((i % 3) == 2)
            {
                if (s[i] != ':')
                    return false;
            }
            else
            {
                char c = s[i];
                bool hex = (c >= '0' && c <= '9') ||
                           (c >= 'a' && c <= 'f') ||
                           (c >= 'A' && c <= 'F');
                if (!hex)
                    return false;
            }
        }
        return true;
    }

    // (tuỳ chọn) normalize schema (thêm khóa còn thiếu / bỏ EMAC 4 octet)
    static bool normalizeBaseConfigJson(DynamicJsonDocument &doc)
    {
        bool changed = false;

        // HostName luôn tồn tại (có thể rỗng)
        if (!doc.containsKey("HostName"))
        {
            doc["HostName"] = "";
            changed = true;
        }

        // EMAC: nếu tồn tại nhưng không phải 6 octet -> xoá để sẽ được ghi đúng từ lệnh CFG MAC
        if (doc.containsKey("EMAC"))
        {
            const char *emac = doc["EMAC"] | "";
            if (!isMacString6Octets(emac))
            {
                doc.remove("EMAC");
                changed = true;
            }
        }

        // SerialID: nếu thiếu, thêm 0
        if (!doc.containsKey("SerialID"))
        {
            doc["SerialID"] = 0;
            changed = true;
        }

        return changed;
    }

    static bool saveMirrorToBaseConfig(const uint8_t *mac /*nullable*/, const char *host /*nullable*/, const uint32_t *devid /*nullable*/)
    {
        if (!beginFS())
            return false;

        DynamicJsonDocument doc(4096);

        // Đọc file hiện tại (nếu có)
        {
            File fr = SPIFFS.open(BASECONFIG, "r");
            if (fr)
            {
                DeserializationError er = deserializeJson(doc, fr);
                fr.close();
                if (er)
                {
                    // lỗi parse -> reset doc rỗng
                    doc.clear();
                    //Serial.printf("[CFG] WARN: parse %s failed (%s), recreating...\n", BASECONFIG, er.c_str());
                }
            }
        }

        // Chuẩn hoá schema (an toàn)
        normalizeBaseConfigJson(doc);

        // --- Merge 3 field quan trọng ---
        if (mac)
        {
            char macs[20] = {0};
            macToStr(mac, macs); // bảo đảm dạng 6 octet
            doc["EMAC"] = macs;
        }
        // host: cho phép host == "" để thể hiện clear
        if (host)
        {
            doc["HostName"] = host;
        }
        // devid: nếu trỏ tới 0 => set 0 (clear)
        if (devid)
        {
            doc["SerialID"] = *devid;
        }

        // Atomic write
        const char *tmpPath = "/baseconfig.tmp";
        File fw = SPIFFS.open(tmpPath, "w");
        if (!fw)
        {
            //Serial.printf("[CFG] ERR: open %s for write failed\n", tmpPath);
            return false;
        }
        bool ok = (serializeJsonPretty(doc, fw) > 0);
        fw.flush();
        fw.close();
        if (!ok)
        {
            //Serial.printf("[CFG] ERR: serialize JSON to %s failed\n", tmpPath);
            SPIFFS.remove(tmpPath);
            return false;
        }

        SPIFFS.remove(BASECONFIG);
        if (!SPIFFS.rename(tmpPath, BASECONFIG))
        {
            // fallback: copy
            File fr2 = SPIFFS.open(tmpPath, "r");
            File fw2 = SPIFFS.open(BASECONFIG, "w");
            if (fr2 && fw2)
            {
                while (fr2.available())
                    fw2.write(fr2.read());
                fw2.flush();
                fw2.close();
                fr2.close();
                SPIFFS.remove(tmpPath);
                return true;
            }
            if (fr2)
                fr2.close();
            if (fw2)
                fw2.close();
            SPIFFS.remove(tmpPath);
            //Serial.printf("[CFG] ERR: rename %s -> %s failed\n", tmpPath, BASECONFIG);
            return false;
        }
        Config_Device.checkBaseConfig();
        return true;
    }

    // ====== API công khai ======
    void setEthReinitHook(EthReinitHook hook) { s_ethReinitHook = hook; }

    bool getSavedMac(uint8_t outMac[6]) { return loadMacRaw(outMac); }

    bool getSavedHostname(String &outHost)
    {
        char buf[32];
        loadHostRaw(buf);
        if (buf[0] == 0)
            return false;
        outHost = String(buf);
        return outHost.length() > 0;
    }

    bool getSavedDeviceId(uint32_t &outId)
    {
        outId = loadDevIdRaw();
        if (outId == 0 || outId == 0xFFFFFFFF)
            return false;
        return true;
    }

    void loadAndApplyEarlyMAC(const uint8_t *defaultMac)
    {
        uint8_t mac[6];
        if (loadMacRaw(mac))
        {
            mac[0] &= ~0x01; // unicast
            esp_base_mac_addr_set(mac);

            // mirror vào cấu trúc & file
            char macstr[32] = {0};
            macToStr(mac, macstr);
            strncpy(Config_Internet.Ethernet.Mac, macstr, sizeof(Config_Internet.Ethernet.Mac) - 1);
            saveMirrorToBaseConfig(mac, nullptr, nullptr);

            //Serial.printf("[CFG] Loaded MAC (EEPROM): %s\n", macstr);
            return;
        }
        if (defaultMac)
        {
            uint8_t d[6];
            memcpy(d, defaultMac, 6);
            d[0] &= ~0x01;
            esp_base_mac_addr_set(d);

            char macstr[32] = {0};
            macToStr(d, macstr);
            strncpy(Config_Internet.Ethernet.Mac, macstr, sizeof(Config_Internet.Ethernet.Mac) - 1);
            // không mirror default vào file (tuỳ bạn). Nếu muốn, bật dòng dưới:
            // saveMirrorToBaseConfig(d, nullptr, nullptr);

            //Serial.printf("[CFG] Using default MAC: %s\n", macstr);
        }
    }

    void applyHostnameIfAny()
    {
        String host;
        if (getSavedHostname(host) && validHostname(host.c_str()))
        {
            // cập nhật cấu trúc
            strncpy(Config_Device.Device.HostName, host.c_str(), sizeof(Config_Device.Device.HostName) - 1);

            // re-init ETH nếu có hook, để DHCP thấy hostname mới
            if (s_ethReinitHook)
                s_ethReinitHook();

            ETH.setHostname(host.c_str());
            saveMirrorToBaseConfig(nullptr, host.c_str(), nullptr);

            //Serial.printf("[CFG] Hostname applied: %s\n", host.c_str());
        }
    }

    void applyDeviceIdIfAny(uint32_t *outApplied)
    {
        uint32_t id;
        if (getSavedDeviceId(id))
        {
            Config_Device.Device.SerialID = id;
            if (outApplied)
                *outApplied = id;

            saveMirrorToBaseConfig(nullptr, nullptr, &id);
            //Serial.printf("[CFG] DeviceID applied: %lu\n", id);
        }
    }

    void showConfig(Stream &out)
    {
        uint8_t mac[6] = {0};
        char host[32] = {0};
        uint32_t id = 0xFFFFFFFF;

        bool hasMac = loadMacRaw(mac);
        loadHostRaw(host);
        id = loadDevIdRaw();

        char macs[20] = {0};
        if (hasMac)
            macToStr(mac, macs);

        out.println(F("=== CFG (EEPROM first) ==="));
        out.printf("MAC(Saved)   : %s\n", hasMac ? macs : "(none)");
        out.printf("HOST(Saved)  : %s\n", host[0] ? host : "(none)");
        out.printf("DEVID(Saved) : %s\n", (id != 0 && id != 0xFFFFFFFF) ? String(id).c_str() : "(none)");
        out.println(F("--- Current (struct) ---"));
        out.printf("MAC(Current) : %s\n", Config_Internet.Ethernet.Mac[0] ? Config_Internet.Ethernet.Mac : "(unknown)");
        out.printf("HOST(Current): %s\n", Config_Device.Device.HostName);
        out.printf("DEVID(Current): %lu\n", (unsigned long)Config_Device.Device.SerialID);
        out.println(F("========================="));
    }

    void clearConfig()
    {
        ensureEEPROM();
        // xoá 3 vùng
        for (int i = 0; i < 6; i++)
            EEPROM.write(EEPROM_ADDR_MAC + i, 0x00);
        for (int i = 0; i < 32; i++)
            EEPROM.write(EEPROM_ADDR_HOST + i, 0x00);
        uint32_t ff = 0xFFFFFFFF;
        EEPROM.put(EEPROM_ADDR_DEVID, ff);
        EEPROM.commit();

        // không bắt buộc xoá trong file; chỉ mirror trống 2 field Host/ID
        // saveMirrorToBaseConfig(nullptr, "", nullptr);
        uint32_t zero = 0;
        saveMirrorToBaseConfig(nullptr, "", &zero);

        //Serial.println("[CFG] Cleared EEPROM config");
    }

    // ====== xử lý lệnh ======
    static void handleCfgMac(const char *macStr)
    {
        uint8_t mac[6];
        if (!parseMac(macStr, mac))
        {
            Serial.println("ACK:CFG,ERR,BAD_MAC");
            return;
        }
        // đặt locally-admin (nếu muốn quản lý nội bộ), và chắc chắn unicast
        mac[0] |= 0x02;  // locally-administered
        mac[0] &= ~0x01; // unicast

        // 1) LƯU EEPROM TRƯỚC
        saveMacRaw(mac);

        // 2) MIRROR RA baseconfig.txt
        saveMirrorToBaseConfig(mac, nullptr, nullptr);

        // 3) cập nhật cấu trúc (để show ngay)
        char macs[20] = {0};
        macToStr(mac, macs);
        strncpy(Config_Internet.Ethernet.Mac, macs, sizeof(Config_Internet.Ethernet.Mac) - 1);
        Serial.printf("ACK:CFG,OK (MAC saved: %s)\n", macs);
        // Serial.printf("ACK:CFG,OK (MAC saved: %s)\n", macs);
        // Serial.println("Rebooting to apply MAC...");
        // delay(200);
        // ESP.restart(); // áp dụng từ rất sớm
        // uint8_t savedMac[6];
        // if (eepromReadMac(savedMac))
        // {
        //     // đảm bảo gọi TRƯỚC ETH.begin()
        //     if (!setCustomMACEarly(savedMac))
        //     {
        //         Serial.println("WARN: setCustomMACEarly(EEPROM) failed, dùng eFuse/base mặc định.");
        //     }
        // }
        // else
        {

            dbg_main("Rebooting to apply MAC...");
            delay(200);
            ESP.restart(); // áp dụng từ rất sớm
        }

        // handle_ethernet.eth_setup();

        // String savedHost;
        // if (eepromReadHostname(savedHost))
        // {
        //     setEthHostname(savedHost.c_str()); // dùng hàm trong mac_hostname_u6.h
        //     Serial.printf("[CFG] Loaded Hostname (EEPROM): %s\n", savedHost.c_str());
        // }
        // else
        // {
        //     Serial.printf("[CFG] Using default Hostname:\n");
        // }
    }

    static void handleCfgHost(const String &hostIn)
    {
        String host = hostIn;
        host.trim();
        if (!validHostname(host.c_str()))
        {
            Serial.println("ACK:CFG,ERR,BAD_HOST");
            return;
        }

        // 1) EEPROM trước
        saveHostRaw(host.c_str());

        // 2) MIRROR file
        saveMirrorToBaseConfig(nullptr, host.c_str(), nullptr);

        // 3) áp dụng runtime & cấu trúc
        strncpy(Config_Device.Device.HostName, host.c_str(), sizeof(Config_Device.Device.HostName) - 1);
        if (s_ethReinitHook)
            s_ethReinitHook();
        ETH.setHostname(host.c_str());

        Serial.printf("ACK:CFG,OK (HOST=%s)\n", host.c_str());

        dbg_main("Rebooting to apply MAC...");
        delay(200);
        ESP.restart(); // áp dụng từ rất sớm
        // handle_ethernet.eth_setup();

        // String savedHost;
        // if (eepromReadHostname(savedHost))
        // {
        //     setEthHostname(savedHost.c_str()); // dùng hàm trong mac_hostname_u6.h
        //     Serial.printf("[CFG] Loaded Hostname (EEPROM): %s\n", savedHost.c_str());
        // }
        // else
        // {
        //     Serial.printf("[CFG] Using default Hostname:\n");
        // }
    }

    static void handleCfgDevid(const char *p)
    {
        uint32_t id = strtoul(p, nullptr, 10);
        if (id == 0)
        {
            Serial.println("ACK:CFG,ERR,BAD_ID");
            return;
        }

        // 1) EEPROM trước
        saveDevIdRaw(id);

        // 2) MIRROR file
        saveMirrorToBaseConfig(nullptr, nullptr, &id);

        // 3) áp dụng runtime
        Config_Device.Device.SerialID = id;

        Serial.printf("ACK:CFG,OK (DEVID=%lu)\n", id);
        // 4) ĐẨY sang ESP32-B
        bool pushed = pushAndConfirmPeerDeviceId(id, /*doConfirm=*/true);

        // 5) ACK
        if (pushed)
        {
            Serial.printf("ACK:CFG,OK (DEVID=%lu, PUSHED_TO_B=OK)\n", id);
        }
        else
        {
            Serial.printf("ACK:CFG,OK (DEVID=%lu, PUSHED_TO_B=FAIL)\n", id);
        }
    }

    // Gộp MAC + HOST (+ optional DEVID)
    static void handleCfgNet(const char *args)
    {
        if (!args)
        {
            Serial.println("ACK:CFG,ERR,BAD_FORMAT");
            return;
        }

        // tách theo dấu phẩy
        String s = args;
        s.trim();
        int c1 = s.indexOf(',');
        if (c1 < 0)
        {
            Serial.println("ACK:CFG,ERR,BAD_FORMAT (use: CFG NET=MAC,HOST[,DEVID])");
            return;
        }
        String macStr = s.substring(0, c1);
        macStr.trim();
        String rest = s.substring(c1 + 1);
        rest.trim();

        String hostStr, idStr;
        int c2 = rest.indexOf(',');
        if (c2 < 0)
        {
            hostStr = rest;
            idStr = ""; // không có devid
        }
        else
        {
            hostStr = rest.substring(0, c2);
            hostStr.trim();
            idStr = rest.substring(c2 + 1);
            idStr.trim();
        }

        // --- validate MAC ---
        uint8_t emac[6];
        if (!parseMac(macStr.c_str(), emac))
        {
            Serial.println("ACK:CFG,ERR,BAD_MAC");
            return;
        }
        // đảm bảo unicast, KHÔNG ép LAA nếu bạn không muốn
        emac[0] &= ~0x01;

        // --- validate HOST ---
        if (!validHostname(hostStr.c_str()))
        {
            Serial.println("ACK:CFG,ERR,BAD_HOST");
            return;
        }

        // --- optional DEVID ---
        bool haveId = false;
        uint32_t devid = 0;
        if (idStr.length() > 0)
        {
            devid = strtoul(idStr.c_str(), nullptr, 10);
            if (devid == 0)
            {
                Serial.println("ACK:CFG,ERR,BAD_ID");
                return;
            }
            haveId = true;
        }

        // 1) GHI EEPROM TRƯỚC
        saveMacRaw(emac);
        saveHostRaw(hostStr.c_str());
        if (haveId)
            saveDevIdRaw(devid);

        // 2) MIRROR file /baseconfig.txt
        saveMirrorToBaseConfig(emac, hostStr.c_str(), haveId ? &devid : nullptr);

        // 3) ÁP DỤNG RUNTIME (host & devid áp dụng ngay; MAC cần reboot)
        char macs[20] = {0};
        macToStr(emac, macs);

        strncpy(Config_Internet.Ethernet.Mac, macs, sizeof(Config_Internet.Ethernet.Mac) - 1);
        strncpy(Config_Device.Device.HostName, hostStr.c_str(), sizeof(Config_Device.Device.HostName) - 1);
        if (haveId)
        {
            Config_Device.Device.SerialID = devid;
        }

        // hostname có thể áp dụng ngay
        ETH.setHostname(hostStr.c_str());
        if (s_ethReinitHook)
            s_ethReinitHook(); // nếu bạn muốn renew DHCP

        if (haveId)
        {
            Serial.printf("ACK:CFG,OK (MAC=%s, HOST=%s, DEVID=%lu)\n", macs, hostStr.c_str(), (unsigned long)devid);
        }
        else
        {
            Serial.printf("ACK:CFG,OK (MAC=%s, HOST=%s)\n", macs, hostStr.c_str());
        }

        // 3.5) ĐẨY DEVID sang ESP32-B trước khi reboot (nếu có)
        if (haveId)
        {
            bool pushed = pushAndConfirmPeerDeviceId(devid, /*doConfirm=*/true);
            dbg_main("[CFG] Push DEVID to B: %s\n", pushed ? "OK" : "FAIL");
            // Không chặn reboot nếu FAIL (tuỳ yêu cầu, bạn có thể return ở đây nếu muốn ép thành công)
        }

        // 4) MAC chỉ áp dụng sau reboot (esp32 base MAC cần set trước ETH.begin)
        dbg_main("Rebooting to apply EMAC (1 sec)...");
        ESPRebootTo.ToEUpdate(1000);
    }

    void handleLine(const String &lineIn)
    {
        String line = lineIn;
        line.trim();
        if (!line.startsWith("CFG "))
            return;

        if (line.startsWith("CFG NET="))
        {
            handleCfgNet(line.c_str() + 8);
            return;
        }

        if (line.startsWith("CFG MAC="))
        {
            handleCfgMac(line.c_str() + 8);
            return;
        }
        if (line.startsWith("CFG HOST="))
        {
            handleCfgHost(line.substring(9));
            return;
        }
        if (line.startsWith("CFG DEVID="))
        {
            handleCfgDevid(line.c_str() + 10);
            return;
        }
        if (line.equals("CFG SHOW"))
        {
            showConfig(Serial);
            Serial.println("ACK:CFG,OK");
            return;
        }
        if (line.equals("CFG CLEAR"))
        {
            clearConfig();
            Serial.println("ACK:CFG,OK");
            return;
        }

        Serial.println("ACK:CFG,ERR,UNKNOWN_CMD");
    }

    void pollSerial(uint32_t readTimeoutMs)
    {
        if (!Serial.available())
            return;
        Serial.setTimeout(readTimeoutMs);
        String line = Serial.readStringUntil('\n');
        if (line.length() == 0)
            return;
        handleLine(line);
    }

} // namespace UartConfig
