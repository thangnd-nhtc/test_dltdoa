#include "button_control.h"
#include "define.h"

// Thời gian chống dội và ngưỡng nhấn giữ (ms)
static const uint16_t DEBOUNCE_MS = 30;
static const uint16_t LONG_PRESS_MS = 700;

// Trạng thái đọc thô & debounced
static int lastReading = HIGH; // lần đọc trước (thô)
static int stableState = HIGH; // trạng thái đã ổn định (debounced)
static unsigned long lastChangeMs = 0;

// Biến phục vụ long-press
static unsigned long pressedAtMs = 0;
static bool longPressFired = false;

void sendUART_PTB(uint8_t *bufferSend, uint32_t len)
{
    // dùng write(buf, len) để gửi mảng bytes một lần
    Serial1.write(bufferSend, len);
}

// gửi trạng thái nút theo định dạng BTN:<0|1>\n
// static inline void sendSwitchStateOverUart(bool pressed)
// {
//     // pressed = true -> 1 ; false -> 0
//     char msg[16];
//     int n = snprintf(msg, sizeof(msg), "BTN:%d\n", pressed ? 1 : 0);
//     sendUART_PTB((uint8_t *)msg, (uint32_t)n);
// }

// ===== Helper: đọc một dòng từ Serial1 có timeout =====
static bool uart1_read_line(String &out, uint32_t timeout_ms)
{
    out = "";
    uint32_t t0 = millis();
    while (millis() - t0 < timeout_ms)
    {
        while (Serial1.available())
        {
            char c = (char)Serial1.read();
            if (c == '\n')
            {
                // cắt \r nếu có
                if (out.length() && out[out.length() - 1] == '\r')
                    out.remove(out.length() - 1);
                return true;
            }
            out += c;
            if (out.length() > 160)
            {
                out = "";
            } // tránh tràn khi nhiễu
        }
        // yield(); // bật nếu cần
    }
    return false; // timeout
}

// ===== Gửi BTN:<0|1>,<SEQ>\n + chờ ACK:<SEQ>,OK (giữ nguyên chữ ký) =====
// Nếu bạn muốn biết kết quả, có thể thay kiểu trả về thành bool và `return ok;`
static inline void sendSwitchStateOverUart(bool pressed)
{
    static uint16_t seq = 0;
    seq++;

    // Soạn bản tin có SEQ
    char msg[24];
    int n = snprintf(msg, sizeof(msg), "BTN:%d,%u\n", pressed ? 1 : 0, (unsigned)seq);

    const int retries = 3;
    const uint32_t ack_timeout_ms = 150;
    bool ok = false;

    for (int attempt = 0; attempt < retries && !ok; ++attempt)
    {
        // Gửi lệnh
        sendUART_PTB((uint8_t *)msg, (uint32_t)n);

        // Chờ ACK khớp SEQ: "ACK:<seq>,OK" hoặc "ACK:<seq>,ERR"
        uint32_t t0 = millis();
        while (millis() - t0 < ack_timeout_ms)
        {
            String line;
            if (uart1_read_line(line, ack_timeout_ms))
            {
                uint16_t rseq = 0;
                char status[8] = {0};
                if (sscanf(line.c_str(), "ACK:%hu,%7s", &rseq, status) == 2)
                {
                    if (rseq == seq)
                    {
                        ok = (strcmp(status, "OK") == 0);
                        break;
                    }
                }
                // Bản tin khác -> bỏ qua
            }
        }
        // nếu chưa ok sẽ retry
    }

    // if (!ok)
    // {
    //     Serial.println("[ESP32 A] ERR: Không nhận ACK hoặc ACK lỗi!");
    //     // (tuỳ chọn) bật LED cảnh báo / đặt cờ để gửi lại nền
    // }
}
// -----------------------------------------------
void setup_button_control()
{
    pinMode(switch_control_led, INPUT_PULLUP);
    dbg_main("Button ready (debounce + long press)");

    stableState = digitalRead(switch_control_led);
    lastReading = stableState;
    // sendSwitchStateOverUart(stableState);
}

void read_button_state(void)
{
    int state = digitalRead(switch_control_led);
    sendSwitchStateOverUart(state);
}

// void loop_button_control()
// {
//     //lastChangeMs = 0;
//     //pressedAtMs = 0;
//     //longPressFired = false;

//     // Đọc hiện tại
//     int reading = digitalRead(switch_control_led);

//     // Phát hiện thay đổi tức thời -> reset bộ đếm chống dội
//     if (reading != lastReading)
//     {
//         lastChangeMs = millis();
//         lastReading = reading;
//     }

//     // Khi vượt qua thời gian chống dội, chấp nhận trạng thái mới là "ổn định"
//     if ((millis() - lastChangeMs) > DEBOUNCE_MS)
//     {
//         if (reading != stableState)
//         {
//             stableState = reading;

//             if (stableState == LOW)
//             {
//                 // Vừa xác nhận NHẤN XUỐNG
//                 //pressedAtMs = millis();
//                 //longPressFired = false;
//                 // Nếu muốn bắt sự kiện "press" ngắn hạn, có thể xử lý ở đây
//                 Serial.println("PRESS");
//                 sendSwitchStateOverUart(true);
//             }
//             else
//             {
//                 // Vừa xác nhận NHẢ NÚT
//                 // unsigned long held = millis() - pressedAtMs;
//                 // if (!longPressFired && held < LONG_PRESS_MS)
//                 // {
//                 //     // NHẤN NGẮN (short press)
//                 //     Serial.println("SHORT PRESS");
//                 // }
//                 // Nếu cần sự kiện "release", xử lý thêm ở đây
//                 Serial.println("RELEASE");
//                 sendSwitchStateOverUart(false);
//             }
//         }
//     }

//     // // Kiểm tra LONG PRESS một lần duy nhất trong lúc đang giữ
//     // if (stableState == LOW && !longPressFired)
//     // {
//     //     if (millis() - pressedAtMs >= LONG_PRESS_MS)
//     //     {
//     //         longPressFired = true;
//     //         Serial.println("LONG PRESS");
//     //         // Thực thi hành động cho nhấn giữ tại đây
//     //     }
//     // }
// }

void loop_button_control()
{
    // Đọc raw hiện tại
    int reading = digitalRead(switch_control_led);

    // Nếu raw thay đổi -> reset bộ đếm chống dội
    if (reading != lastReading)
    {
        lastReading = reading;
        lastChangeMs = millis();
    }

    // Khi vượt qua khoảng chống dội, chấp nhận raw là trạng thái ổn định mới
    if ((millis() - lastChangeMs) > DEBOUNCE_MS)
    {
        if (reading != stableState)
        {
            // CẠNH THAY ĐỔI TRẠNG THÁI ỔN ĐỊNH
            stableState = reading;

            if (stableState == LOW)
            {
                // NHẤN XUỐNG (active low)
                dbg_main("PRESS");
                sendSwitchStateOverUart(false);
            }
            else
            {
                // NHẢ NÚT
                dbg_main("RELEASE");
                sendSwitchStateOverUart(true);
            }
        }
    }
}
