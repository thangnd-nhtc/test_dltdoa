#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include <ETH.h>
#include <WiFi.h>

// ---- Cấu hình vùng EEPROM (tổng < 128 byte) ----
constexpr size_t EEPROM_SIZE = 128;
constexpr int EEPROM_ADDR_MAC = 0;    // 6 byte
constexpr int EEPROM_ADDR_HOST = 6;   // 32 byte (chuỗi, có '\0')
constexpr int EEPROM_ADDR_DEVID = 38; // 4 byte (uint32_t)

//
void saveDevIdRaw(uint32_t id);
bool pushAndConfirmPeerDeviceId(uint32_t id, bool doConfirm = true);
bool pushPeerDeviceId(uint32_t id,
                      uint8_t maxRetries = 3,
                      uint32_t respTimeoutMs = 800);
bool queryPeerDeviceId(uint32_t &outId,
                       uint8_t maxLines = 3,
                       uint32_t respTimeoutMs = 600);

bool eepromReadMac(uint8_t out[6]);
bool eepromReadHostname(String &host);
bool eepromReadDeviceID(uint32_t &outId);

namespace UartConfig
{

    // ---- API tích hợp hệ thống ----
    // Gọi CÀNG SỚM CÀNG TỐT: áp dụng MAC từ EEPROM (nếu có), hoặc dùng default nếu bạn truyền vào
    void loadAndApplyEarlyMAC(const uint8_t *defaultMac = nullptr);

    // Sau khi hệ thống sẵn sàng, áp dụng Hostname/DeviceID nếu đã lưu
    void applyHostnameIfAny();
    void applyDeviceIdIfAny(uint32_t *outApplied = nullptr);

    // Poll cổng Serial để đọc & xử lý lệnh CFG ... (gọi định kỳ trong loop/task debug)
    void pollSerial(uint32_t readTimeoutMs = 10);

    // Xử lý trực tiếp một dòng lệnh (nếu bạn tự đọc Serial nơi khác)
    void handleLine(const String &line);

    // ---- Hook tuỳ chọn: bạn có thể đăng ký hàm re-init Ethernet của bạn ----
    using EthReinitHook = void (*)(); // ví dụ: tắt WiFi, toggle GPIO5, ETH.begin(...)
    void setEthReinitHook(EthReinitHook hook);

    // ---- Tiện ích: xem/clear cấu hình đã lưu ----
    void showConfig(Stream &out);
    void clearConfig();

    // ---- Getters: đọc giá trị đang lưu trong EEPROM (không áp dụng) ----
    bool getSavedMac(uint8_t outMac[6]);    // trả true nếu có dữ liệu hợp lệ
    bool getSavedHostname(String &outHost); // true nếu có host != ""
    bool getSavedDeviceId(uint32_t &outId); // true nếu id != 0xFFFFFFFF && != 0

    // ---- Helpers kiểm tra/parse ----
    bool parseMac(const char *s, uint8_t mac[6]); // "AA:BB:CC:DD:EE:FF"
    bool validHostname(const char *host);

} // namespace UartConfig
