#include "reset_factory_dhcp.h"
#include "define.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>


/* ============================================================
 *   GHI FILE CẤU HÌNH DHCP MẶC ĐỊNH
 * ============================================================ */

static bool writeTextFile(const char* path, const String& content) {
  if (SPIFFS.exists(path)) {
    SPIFFS.remove(path);
  }

  File f = SPIFFS.open(path, FILE_WRITE);
  if (!f) {
    debug_SPIFFS("[reset_factory_dhcp] Open %s for write failed\n", path);
    return false;
  }

  size_t n = f.print(content);
  f.close();

  if (n != content.length()) {
    debug_SPIFFS("[reset_factory_dhcp] Write %s incomplete (%u/%u)\n",
                  path, (unsigned)n, (unsigned)content.length());
    return false;
  }

  return true;
}

bool reset_config_DHCP(const char* path, bool autoMount) {
  if (autoMount) {
    if (!SPIFFS.begin(true)) { // true = format nếu mount thất bại
      debug_SPIFFS("[reset_factory_dhcp] SPIFFS.begin() failed");
      return false;
    }
  }

  // JSON mặc định
  DynamicJsonDocument doc(256);
  doc["DhcpMode"] = 1;
  doc["IP"]       = "192.168.0.100";
  doc["GW"]       = "192.168.0.1";
  doc["Sn"]       = "255.255.255.0";
  doc["DNS"]      = "8.8.8.8";

  String payload;
  serializeJson(doc, payload);

  if (!writeTextFile(path, payload)) {
    debug_SPIFFS("[reset_factory_dhcp] Write default DHCP config failed");
    return false;
  }

  debug_SPIFFS("[reset_factory_dhcp] Default DHCP config saved to %s: %s\n",
                path, payload.c_str());
  return true;
}

/* ============================================================
 *   XỬ LÝ NÚT NHẤN RESET FACTORY (GPIO2 - ACTIVE-LOW)
 * ============================================================ */

// Biến trạng thái nút & cấu hình chống dội/giữ
static uint8_t  s_btnPin         = 2;
static uint16_t s_debounceMs     = 30;
static uint32_t s_longPressMs    = 10000;

static bool     s_rawState       = true;   // đọc trực tiếp digitalRead (true = HIGH)
static bool     s_debounced      = false;  // đã chống dội (true = đang nhấn)
static bool     s_lastDebounced  = false;

static uint32_t s_lastChangeMs   = 0;      // mốc thời gian thay đổi gần nhất (raw)
static uint32_t s_pressStartMs   = 0;      // mốc bắt đầu nhấn (debounced)
static bool     s_firedLongPress = false;  // đã gọi onFactoryButtonLongPress() cho lần nhấn hiện tại

// Đọc raw & cập nhật chống dội
static void buttonDebounceUpdate() {
  bool rawNow = digitalRead(s_btnPin);                 // HIGH = thả (vì INPUT_PULLUP)
  uint32_t now = millis();

  if (rawNow != s_rawState) {
    s_rawState = rawNow;
    s_lastChangeMs = now;                              // vừa có thay đổi raw, bắt đầu đếm debounce
  }

  // Sau khi raw ổn định đủ debounceMs, cập nhật debounced
  if ((now - s_lastChangeMs) >= s_debounceMs) {
    bool pressed = (s_rawState == LOW);                // active-low
    if (pressed != s_debounced) {
      s_debounced = pressed;
      // sự kiện cạnh
      if (s_debounced) {
        // Bắt đầu nhấn
        s_pressStartMs   = now;
        s_firedLongPress = false;
      } else {
        // Vừa thả nút
        s_pressStartMs   = 0;
        s_firedLongPress = false;
      }
    }
  }
}

void factoryButtonBegin(uint8_t pin, uint16_t debounceMs, uint32_t longPressMs) {
  s_btnPin      = pin;
  s_debounceMs  = debounceMs;
  s_longPressMs = longPressMs;

  pinMode(s_btnPin, INPUT_PULLUP);   // Nút nối GND
  s_rawState       = digitalRead(s_btnPin);
  s_debounced      = (s_rawState == LOW);
  s_lastDebounced  = s_debounced;
  s_lastChangeMs   = millis();
  s_pressStartMs   = s_debounced ? millis() : 0;
  s_firedLongPress = false;

  // Cảnh báo nhẹ: GPIO2 là strapping pin, không nên giữ LOW khi khởi động
  // Serial.printf("[reset_factory_dhcp] Button on GPIO%u (INPUT_PULLUP), debounce=%ums, longPress=%ums\n",
  //               s_btnPin, s_debounceMs, s_longPressMs);
}

bool factoryButtonIsPressed() {
  return s_debounced;
}

uint32_t factoryButtonHoldMs() {
  if (!s_debounced) return 0;
  uint32_t now = millis();
  return (now - s_pressStartMs);
}

static void reset_U7()
{ // Mở lại để reset lại U7 khi bị treo
  pinMode(pin_reset_u7, OUTPUT);
  digitalWrite(pin_reset_u7, LOW);
  digitalWrite(pin_reset_u7, LOW);
  delay(200);
  pinMode(pin_reset_u7, INPUT);
  debug_SPIFFS("\r\nRESET u7\r\n");
}

void onFactoryButtonLongPress() __attribute__((weak));
void onFactoryButtonLongPress() {
  // Mặc định: ghi cấu hình DHCP và in log. Bạn có thể override ở sketch.
  debug_SPIFFS("[reset_factory_dhcp] Long-press detected >= threshold, resetting DHCP config...");
  if (reset_config_DHCP()) {
    debug_SPIFFS("[reset_factory_dhcp] DHCP default written. (Override onFactoryButtonLongPress() to add custom behavior, e.g., ESP.restart())");
    reset_U7();
    ESP.restart();
  } else {
    debug_SPIFFS("[reset_factory_dhcp] DHCP default write FAILED.");
  }
}

void factoryButtonTask() {
  buttonDebounceUpdate();

  // Nếu đang nhấn & chưa bắn sự kiện long-press
  if (s_debounced && !s_firedLongPress) {
    uint32_t held = factoryButtonHoldMs();
    if (held >= s_longPressMs) {
      s_firedLongPress = true;
      onFactoryButtonLongPress();
    }
  }

  // Lưu lại lần trước (hiện chưa dùng, nhưng để sẵn nếu cần detect edge)
  s_lastDebounced = s_debounced;
}
