/** CHƯƠNG TRÌNH XỬ LÝ DECAWAVE CHO BASE 03
 * 1. nhận tag
 * 2. nhận sync_rx không dây
 * 3. sync_rx wire
 * 4. setup thông số qua spi
 * 5. gửi thông số tag lên server qua spi
 * 6. led status được điều khiển qua spi
 *
 * 7. phát beacon
 * 8. tinh twr
 * SERIAL_ID_RES
:$02$1D$AE$7D$00$30$08$00$53$45$52$49$41$4C$5F$49$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
MASTER_RES
:$02$1D$82$FC$01$30$08$00$4D$41$53$54$45$52$5F$52$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
MASTER_ACCESS_RES
:$02$1D$75$FD$02$30$10$00$4D$41$53$54$45$52$5F$41$43$43$45$53$53$5F$52$45$53$00$44$57$5F$43$4F$4E$46$49$47$5F$52$45$53$00$44$57$5F$43$4F$4E$46$49$47$5F$54$58$5F$52$45$53$00$44$57$5F$41$4E$54$5F$44$45$4C$41$59$5F$52$45$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
DW_CONFIG_RES
:$02$1D$74$32$03$30$0C$00$44$57$5F$43$4F$4E$46$49$47$5F$52$45$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
DW_CONFIG_TX_RES
:$02$1D$AE$18$04$30$08$00$44$57$5F$43$4F$4E$46$49$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
DW_ANT_DELAY_RES
:$02$1D$0F$68$05$30$04$00$44$57$5F$41$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
LAST_UPDATE_RES
:$02$1D$17$30$06$30$04$00$4C$41$53$54$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
HANDLE_LED_STATUS_RES
:$04$1D$BD$39$00$10$03$00$05$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
HANDLE_LAST_UPDATE_RES
:$04$1D$89$11$01$10$01$00$01$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
 * **/
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IOBlink.h>
#include <IOInput.h>
#include <SPIFFS.h>
#include <TimeOutEvent.h>
#include <Update.h>
#include <esp_task_wdt.h>

#include "main.h"

#include "Tools.h"
#include "config.h"
#include "define.h"
#include "handle_ISP3080.h"
#include "handle_com.h"
#include "handle_decawave.h"
#include "handle_spifs.h"
#include "handle_spis.h"
#include "handle_status.h"
#include <EEPROM.h>
#include <WiFi.h>

TaskHandle_t MainCore0;
TaskHandle_t Dw1000Core1;
uint8_t debug_TDOA;

volatile bool flag_timeout_led = false;
unsigned long timeout_on_led =
    30000;                            // thời gian timeout hiển thị led theo trạng thái công tắc
unsigned long millis_on_led = 0;      // biến lưu thời gian bật led trạng thái
volatile bool stat_btn = false;       // Mặc định là bật led, sau 30s sẽ hiển thị theo
                                      // trạng thái của công tắc
static uint16_t last_seq_handled = 0; // led biến lưu lần cuối đã xử lý seq nào
static bool has_last_seq =
    false; // led biến cho biết đã có last_seq_handled hợp lệ chưa

void printf_hex(char *header, uint8_t *data, unsigned int len)
{
  dbg_main("\r\n%s :", header);
  for (unsigned int i = 0; i < len; i++)
    dbg_main("%d ", data[i]);
  dbg_main("%s", "");
}

volatile uint8_t FlagISP3080OTA =
    0; // Cờ dừng chạy Core 1 riêng cho ISP3080 OTA

void Taks_com_rx(uint8_t *data, uint16_t length)
{
  if (FlagOTA)
  {
    checktimeOTA.ToEUpdate(10000);
    parametter_dw.reciverOTA(data, length);
    return;
  }

  frame_work_t frame_work;
  // coppy vào frame dữ liệu
  memcpy((uint8_t *)&frame_work, data,
         (length < SIZE_FRAME_COMWORK) ? length : SIZE_FRAME_COMWORK);
  // tính toán crc và check
  uint16_t crc =
      Handle_SPIs.calcCRC(frame_work.data, frame_work.header.msk_regs.len);

  if (crc != frame_work.header.check_crc || frame_work.header.type == 0)
  {
    dbg_main("crc error %d-%d, len: %d, add: %X, type: %d", crc,
             frame_work.header.check_crc, frame_work.header.msk_regs.len,
             frame_work.header.msk_regs.add, frame_work.header.type);
    return;
  }

  // ==== [NEW] Xử lý Binary ISP3080 OTA ====
  if (frame_work.header.type == write_ram)
  {
    uint16_t add = frame_work.header.msk_regs.add;
    if (add >= 0x10A0 && add <= 0x10A3)
    {
      bool ok = false;
      if (add == 0x10A0)
      { // ENTER
        uint32_t fw_size = 0, crc32_val = 0;
        memcpy(&fw_size, frame_work.data, 4);
        memcpy(&crc32_val, frame_work.data + 4, 4);
        dbg_main("[U7_OTA_BIN] Nhận ENTER: size=%lu\n", fw_size);
        FlagISP3080OTA = 1;
        checktimeOTA.ToEUpdate(60000); // ISP OTA cần timeout dài hơn OTA U7
        ok = isp3080_ota_enter(fw_size, crc32_val);
        if (ok)
          checktimeOTA.ToEUpdate(60000);
        if (!ok)
          FlagISP3080OTA = 0;
      }
      else if (add == 0x10A1)
      { // DATA
        uint32_t offset = 0;
        uint16_t data_len = 0;
        memcpy(&offset, frame_work.data, 4);
        memcpy(&data_len, frame_work.data + 4, 2);
        checktimeOTA.ToEUpdate(60000); // DATA đang xử lý thì không để watchdog hết giờ
        ok = isp3080_ota_data(offset, frame_work.data + 6, data_len);
        if (ok)
          checktimeOTA.ToEUpdate(60000);
      }
      else if (add == 0x10A2)
      { // END
        checktimeOTA.ToEUpdate(120000); // END có verify/copy/reboot ISP
        ok = isp3080_ota_end();
        FlagISP3080OTA = 0;
      }
      else if (add == 0x10A3)
      { // ABORT
        ok = isp3080_ota_abort();
        FlagISP3080OTA = 0;
      }

      // Phản hồi ACK
      frame_work_t ack;
      memset(&ack, 0, sizeof(ack));
      ack.header.type = read_ram;
      ack.header.msk_regs.add = add;
      ack.header.msk_regs.len = 1;
      ack.data[0] = ok ? 1 : 0;
      ack.header.check_crc = Handle_SPIs.calcCRC(ack.data, 1);
      Serial1.write((uint8_t *)&ack,
                    sizeof(ack.header) + ack.header.msk_regs.len);
      return; // Không chạy tiếp Handle_Com.process_raw_data
    }
  }

  // Cập nhật Power trước khi xử lý frame để phản hồi read trả trạng thái mới nhất.
  led_status.power(ledP_on);
  if (Handle_Com.process_raw_data(&frame_work) == 1)
  {
    memcpy((uint8_t *)&frame_work.data, Handle_Com.com_frame.data,
           Handle_Com.com_frame.header.msk_regs.len);
    frame_work.header.check_crc =
        Handle_SPIs.calcCRC(frame_work.data, frame_work.header.msk_regs.len);
    Serial1.write((uint8_t *)&frame_work,
                  sizeof(frame_work.header) + frame_work.header.msk_regs.len);
    // Handle_SPIs.tranmister((uint8_t *)&frame_work, sizeof(frame_work.header)
    // + frame_work.header.msk_regs.len);
    dbg_main("send crc %d, add %X len: %d", frame_work.header.check_crc,
             frame_work.header.msk_regs.add, frame_work.header.msk_regs.len);
    // 20240522 - open below
    // printf_hex("Reply data", (uint8_t *)&frame_work.data,
    // frame_work.header.msk_regs.len);
  }
}

void Taks_Spi_tx(void)
{
  uint8_t buffer[255];
  memset(buffer, 0, 255);
  int len = Handle_Com.TakeBuff((uint8_t *)buffer);
  if (len > 0)
  {
    Handle_SPIs.tranmister((uint8_t *)buffer, (uint16_t)len + 10);
    if (debug_TDOA == 1)
      dbg_TDoA("%s", buffer);
  }
}

//------------------------------LED ACK
static inline void sendAck(uint16_t seq, bool ok)
{
  // Gửi về cùng UART mà bạn đang dùng để nhận lệnh (Serial1)
  // Định dạng: ACK:<SEQ>,OK hoặc ACK:<SEQ>,ERR
  Serial1.printf("ACK:%u,%s\n", seq, ok ? "OK" : "ERR");
}

// (tuỳ chọn) tách phần thao tác LED để dễ trả kết quả ok/err
static bool apply_button_action(int v)
{
  if (v == 1)
  {
    flag_timeout_led = true;
    stat_btn = true;
    led_status.begin(); // khởi động led status
    if (digitalRead(LOL) || digitalRead(LOS))
    {
      led_status.decawace(led_clock_error_DW);
    }
    else
    {
      led_status.decawace(led_ok_DW);
    }
    return true;
  }
  else if (v == 0)
  {
    // flag_timeout_led = true;
    stat_btn = false;
    // Nếu bạn muốn tắt hẳn status LED khi nhả, gọi API tắt phù hợp ở đây:
    // led_status.end(); // hoặc handle_led_off();
    return true;
  }
  return false; // giá trị không hợp lệ
}
// --- utils: parse uint32 hỗ trợ dec/hex ---
static bool parseUint32(const char *in, uint32_t &out)
{
  if (!in)
    return false;
  // bỏ khoảng trắng đầu
  while (*in == ' ' || *in == '\t')
    in++;

  // hex?
  if (in[0] == '0' && (in[1] == 'x' || in[1] == 'X'))
  {
    char *endp = nullptr;
    unsigned long v = strtoul(in + 2, &endp, 16);
    if (endp == (in + 2))
      return false;
    out = (uint32_t)v;
    return true;
  }

  // dec
  char *endp = nullptr;
  unsigned long v = strtoul(in, &endp, 10);
  if (endp == in)
    return false;
  out = (uint32_t)v;
  return true;
}

// --- utils: helper chuyển hex sang int ---
static inline int hex2int(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

void handleLine(const char *line, size_t len)
{
  // Cắt \r\n ở cuối
  while (len && (line[len - 1] == '\n' || line[len - 1] == '\r'))
    len--;
  if (len == 0)
    return;

  // Copy sang buffer có null-terminator để dùng sscanf/strtok an toàn
  // Buffer mở rộng 600 byte để chứa chuỗi hex dài của OTA (256 byte data = 512
  // char)
  char buf[600];
  size_t n = (len < sizeof(buf) - 1) ? len : (sizeof(buf) - 1);
  memcpy(buf, line, n);
  buf[n] = '\0';

  // [DEBUG] Xem U7 có thực sự bắt được chuỗi text không
  // if (strncasecmp(buf, "ISP_OTA:", 8) == 0 ||
  //     strncasecmp(buf, "BTN:", 4) == 0 || strncasecmp(buf, "DEVID", 5) == 0)
  //     {
  //   Serial.printf(" Nh[U7_LINE_PARSER]an: %s\n", buf);
  // }

  // ========= Forward SIZE_OTA sang reciverOTA khi đang OTA U7 =========
  if (FlagOTA && strncmp(buf, "SIZE_OTA:", 9) == 0)
  {
    checktimeOTA.ToEUpdate(10000); // Giữ OTA session sống khi còn data
    // Serial.printf("[U7_OTA] Forward SIZE_OTA: %s\n", buf);
    parametter_dw.reciverOTA((uint8_t *)buf, n);
    return;
  }

  // ========= [MỚI] LỆNH CẤU HÌNH DEVID =========
  // 1) DEVID?   -> đọc ID
  if (strcasecmp(buf, "DEVID?") == 0)
  {
    uint32_t id = Config::getDeviceID();
    parametter_dw.serial_id.device = id; // đồng bộ biến runtime
    Serial1.printf("OK DEVID=%lu\r\n", (unsigned long)id);
    return;
  }

  // 2) DEVID=<value>  (cho phép khoảng trắng: "DEVID = 123" hoặc
  // "DEVID=0x1E240")
  if (strncasecmp(buf, "DEVID", 5) == 0)
  {
    const char *p = buf + 5;
    while (*p == ' ' || *p == '\t')
      p++;
    if (*p != '=')
    {
      Serial1.print("ERR syntax (use DEVID=<value>)\r\n");
      return;
    }
    p++; // skip '='
    while (*p == ' ' || *p == '\t')
      p++;

    uint32_t newId = 0;
    if (!parseUint32(p, newId))
    {
      Serial1.print("ERR invalid number\r\n");
      return;
    }
    if (newId == 0u || newId == 0xFFFFFFFFu)
    {
      Serial1.print("ERR invalid range (0 or 0xFFFFFFFF not allowed)\r\n");
      return;
    }

    if (!Config::begin())
    {
      Serial1.print("ERR eeprom begin\r\n");
      return;
    }
    Config::setDeviceID(newId, true);
    parametter_dw.serial_id.device = newId; // cập nhật runtime

    Serial1.printf("OK DEVID=%lu\r\n", (unsigned long)newId);
    return;
  }

  // ========= [MỚI] OTA ISP3080 QUA TEXT (Giảm tải độ khó cho U6/U7) =========
  if (strncasecmp(buf, "ISP_OTA:", 8) == 0)
  {
    char *cmd = buf + 8;
    if (strncasecmp(cmd, "ENTER,", 6) == 0)
    {
      uint32_t fw_size = 0;
      uint32_t crc32_val = 0;
      if (sscanf(cmd + 6, "%lu,%lu", &fw_size, &crc32_val) == 2)
      {
        dbg_main("[U7_OTA] Parsed ENTER: size=%lu, crc=%lu\n", fw_size,
                      crc32_val);
        FlagOTA = 1; // STOP Core 1 SPI traffic
        checktimeOTA.ToEUpdate(60000); // ISP OTA cần timeout dài hơn OTA U7
        bool ok = isp3080_ota_enter(fw_size, crc32_val);
        if (ok)
          checktimeOTA.ToEUpdate(60000);
        dbg_main("[U7_OTA] isp3080_ota_enter -> %d\n", ok);
        if (!ok)
          FlagOTA = 0; // Kích hoạt lại Core 1 nếu thất bại
        Serial1.printf("ISP_OTA_ACK:ENTER,%s\n", ok ? "OK" : "ERR");
      }
      else
      {
        dbg_main("[U7_OTA] Sscanf ERR: %s\n", cmd + 6);
        Serial1.printf("ISP_OTA_ACK:ENTER,ERR_PARSE\n");
      }
      return;
    }
    else if (strncasecmp(cmd, "DATA,", 5) == 0)
    {
      uint32_t offset = 0;
      uint32_t data_len = 0;
      // Tìm vị trí chuỗi hex
      char *hex_ptr = nullptr;
      if (sscanf(cmd + 5, "%lu,%lu,", &offset, &data_len) == 2)
      {
        // Tính toán chuỗi hex nằm ở đâu
        const char *p = cmd + 5;
        int commas = 0;
        while (*p && commas < 2)
        {
          if (*p == ',')
            commas++;
          p++;
        }
        hex_ptr = (char *)p;

        // Convert chuỗi HEX -> Binary (Tối đa 256 bytes)
        if (data_len > 256)
        {
          Serial1.printf("ISP_OTA_ACK:DATA,ERR_TOOLONG\n");
          return;
        }
        uint8_t bin[256];
        size_t parsed_len = 0;
        while (hex_ptr[0] && hex_ptr[1] && parsed_len < data_len)
        {
          int h = hex2int(hex_ptr[0]);
          int l = hex2int(hex_ptr[1]);
          if (h < 0 || l < 0)
            break;
          bin[parsed_len++] = (h << 4) | l;
          hex_ptr += 2;
        }

        if (parsed_len == data_len)
        {
          checktimeOTA.ToEUpdate(60000); // DATA đang xử lý thì không để watchdog hết giờ
          bool ok = isp3080_ota_data(offset, bin, data_len);
          if (ok)
            checktimeOTA.ToEUpdate(60000);
          Serial1.printf("ISP_OTA_ACK:DATA,%s\n", ok ? "OK" : "ERR");
        }
        else
        {
          Serial1.printf("ISP_OTA_ACK:DATA,ERR_HEX\n");
        }
      }
      else
      {
        Serial1.printf("ISP_OTA_ACK:DATA,ERR_PARSE\n");
      }
      return;
    }
    else if (strcasecmp(cmd, "END") == 0)
    {
      checktimeOTA.ToEUpdate(120000); // END có thể verify/copy/reboot ISP
      bool ok = isp3080_ota_end();
      FlagOTA = 0; // Kích hoạt lại Core 1
      Serial1.printf("ISP_OTA_ACK:END,%s\n", ok ? "OK" : "ERR");
      return;
    }
    else if (strcasecmp(cmd, "ABORT") == 0)
    {
      bool ok = isp3080_ota_abort();
      FlagOTA = 0; // Kích hoạt lại Core 1
      Serial1.printf("ISP_OTA_ACK:ABORT,%s\n", ok ? "OK" : "ERR");
      return;
    }
  }
  // ========= [HẾT PHẦN MỚI] =========

  // Ưu tiên định dạng mới: BTN:<0|1>,<SEQ>
  int v = -1;
  unsigned seq_u = 0;
  if (sscanf(buf, "BTN:%d,%u", &v, &seq_u) == 2)
  {
    uint16_t seq = (uint16_t)seq_u;

    // Idempotent: nếu SEQ đã xử lý -> chỉ gửi lại ACK, không làm lại
    if (has_last_seq && seq == last_seq_handled)
    {
      sendAck(seq, true);
      return;
    }

    // Thực thi hành động
    bool ok = (v == 0 || v == 1) ? apply_button_action(v) : false;

    // Log giữ nguyên phong cách của bạn
    // Serial.println("State button: " + String(v));

    // Ghi nhận SEQ đã xử lý & phản hồi
    last_seq_handled = seq;
    has_last_seq = true;
    sendAck(seq, ok);
    return;
  }

  // Tương thích định dạng cũ: BTN:<0|1> (không có SEQ)
  if (strncmp(buf, "BTN:", 4) == 0 && strlen(buf) >= 5)
  {
    // Lấy v từ sau "BTN:"
    v = (int)strtol(buf + 4, nullptr, 10);

    bool ok = (v == 0 || v == 1) ? apply_button_action(v) : false;
    // Serial.println("State button: " + String(v));

    // Không có SEQ -> trả ACK với seq=0
    sendAck(0, ok);
    return;
  }

  // (tuỳ chọn) xử lý thêm các lệnh khác ở đây...
}

//------------------------------LED ACK

void Task_Uart_rx(void) // nhận dữ liệu uart U6
{
  // Bộ đệm dòng cho parser ASCII (BTN:/EVT:.../ISP_OTA:...)
  static char lineBuf[600];
  static size_t idx = 0;

  // Đọc theo "chunk" để vừa hiệu quả vừa an toàn
  static const size_t CHUNK = 512;
  static uint8_t buff[CHUNK];

  // === KHI ĐANG OTA U7: reciverOTA() đọc Serial1 trực tiếp (blocking) ===
  // Không được chen vào đọc Serial1 ở đây, tránh tranh chấp dữ liệu!
  if (FlagOTA && FlagWriteOTA)
  {
    return; // Để reciverOTA() tự đọc Serial1
  }

  // Đọc hết những gì đang có trong RX buffer
  while (Serial1.available() > 0)
  {
    // Lấy số byte hiện có, giới hạn theo CHUNK
    size_t avail = Serial1.available();
    if (avail > CHUNK)
      avail = CHUNK;

    // Đọc ra buff (không nên dùng VLA, và không dựa vào len > 255 để flush)
    size_t n = Serial1.readBytes(buff, avail);
    if (n == 0)
      break; // bảo vệ nếu timeout ngắn

    // 1) FEED sang parser ASCII theo dòng (như pollUartLines)
    for (size_t i = 0; i < n; ++i)
    {
      char c = (char)buff[i];

      if (c == '\n')
      {
        // Kết thúc 1 dòng
        lineBuf[idx] = 0;
        handleLine(lineBuf, idx); // xử lý dòng
        idx = 0;                  // reset buffer dòng
        // Sau khi handleLine forward SIZE_OTA → reciverOTA bật FlagWriteOTA
        // → cần dừng đọc ngay, để reciverOTA tự đọc binary data từ Serial1
        if (FlagOTA && FlagWriteOTA)
        {
          return; // Thoát ngay, nhường Serial1 cho reciverOTA
        }
      }
      else
      {
        if (idx < sizeof(lineBuf) - 1)
        {
          lineBuf[idx++] = c;
        }
        else
        {
          // Quá dài -> bỏ dòng hiện tại
          idx = 0;
        }
      }
    }

    // 2) FEED nguyên chunk sang xử lý nhị phân
    // Bỏ qua khi đang OTA U7 (reciverOTA xử lý riêng qua text protocol)
    if (FlagOTA)
    {
      continue; // Không feed binary parser khi đang OTA
    }

    // Accumulator để nhận đủ 1 frame_work_t (vì bị phân mảnh qua UART)
    static uint8_t bin_buf[512];
    static uint16_t bin_idx = 0;
    static uint32_t last_rx_time = 0;

    // Quá 50ms không nhận đủ -> rác -> xóa buffer
    if (bin_idx > 0 && millis() - last_rx_time > 50)
    {
      bin_idx = 0;
    }

    for (size_t i = 0; i < n; i++)
    {
      bin_buf[bin_idx++] = buff[i];
      last_rx_time = millis();

      // size header của struct là 8 byte (đã tính padding)
      if (bin_idx >= sizeof(frame_work_t::header))
      {
        frame_work_t *f = (frame_work_t *)bin_buf;
        uint16_t expected_len = sizeof(f->header) + f->header.msk_regs.len;

        // Nếu type rác (chữ cái ASCII > 10) hoặc độ dài không hợp lý
        if (f->header.type == 0 || f->header.type > 10 ||
            expected_len > sizeof(frame_work_t))
        {
          // Xóa rác, dùng lại buffer từ đầu
          bin_idx = 0;
        }
        else if (bin_idx == expected_len)
        {
          // Đã gom đủ 1 frame -> gửi xử lý
          Taks_com_rx(bin_buf, expected_len);
          bin_idx = 0;
        }
      }
    }
  }
}

void Task_debug_rx(void)
{
  if (Serial.available())
  {
    String data = Serial.readString();
    // Serial.print(data);
    if (strcmp(data.c_str(), "DEBUG=1") == 0)
    {
      dbg_main("%s", "active debug TDoA");
      EEPROM.write(0, 1);
      EEPROM.commit();
      debug_TDOA = 1;
    }
    else if (strcmp(data.c_str(), "DEBUG=0") == 0)
    {
      dbg_main("%s", "deactive debug TDoA");
      EEPROM.write(0, 0);
      EEPROM.commit();
      debug_TDOA = 0;
    }
  }
}

void Init_Setup(void)
{
  millis_on_led = millis(); // khởi tạo mốc thời gian bật led trạng thái
  led_status.begin();
  unsigned long timeout = millis();
  while ((millis() - timeout) < 10000)
  {
    led_status.loop();
    delay(1);
  }
  // //20240622
  delay(100);

  // Serial1.setRxBufferSize(1024 + 10);
  Serial1.setRxBufferSize(4096); // Cấp test Thắng 10/06/26
  Serial1.setTimeout(300);
  Serial1.begin(921600, SERIAL_8N1, 34, 22);
  Serial1.flush();

  Serial.begin(115200);
  // Serial.println("U7 HACHI Megaspeed ACK Offset RST 202512");
  dbg_main("%s", "U7 HACHI 14042026 OTA READY");
  Serial.setTimeout(10);

  // EEPROM.begin(10);
  Config::begin();
  debug_TDOA = EEPROM.read(0);
  dbg_main("debug : %d--", debug_TDOA);
  if (!Config::isDeviceIDValid())
  {
    dbg_main("%s", "Device ID chưa có, ghi mặc định...");
    Config::setDeviceID(000000); // chỉ cần chạy 1 lần
  }

  parametter_dw.SPIFFSbegin();
  // 20240622
  // Handle_Dw.readDW();
  dbg_main("Hardware Ver: %s", HARDWARE_VERSION);
  dbg_main("Firmware Ver: %s", FIRMWARE_VERSION);

  Handle_SPIs.begin();
  // Handle_SPIs.reciver_callback(Taks_Spi_rx);

  if (Handle_Dw.begin() == false)
  {
    // led_status.decawace(led_error_DW);
    dbg_main("decawave init fail\r\n");
  }
  else
  {
    // led_status.decawace(led_ok_DW);
    dbg_main("decawave init OK\r\n");
  }
}

volatile bool g_flag_push_fragment = false;
fragment_status_t g_cached_fragment_status = {0};

volatile bool g_flag_push_request = false;
request_data_t g_cached_request_data = {0};

// ==== BSS-TWR Result: Hàng đợi an toàn đa luồng (Core 1 push, Core 0 pull)
// ====
#define U7_TWR_QUEUE_SIZE 32
dw_twr_result_t g_twr_queue[U7_TWR_QUEUE_SIZE];
volatile int g_twr_q_head = 0;
volatile int g_twr_q_tail = 0;

void codeForTaskMainCore0(void *parameter)
{
  while (1)
  {
    // chương trình xử lý giao tiếp uart
    Task_Uart_rx();
    Task_debug_rx();
    if (millis() - millis_on_led >= timeout_on_led &&
        flag_timeout_led == false)
    {
      flag_timeout_led = true;
      dbg_main("%s", "timeout led 30s status");
    }

    if (!flag_timeout_led)
    {
      led_status.loop();
    }
    else
    {
      if (stat_btn)
      {
        led_status.loop();
      }
      else
      {
        led_status.off_all();
      }
    }

    Handle_Com.loop_polling_fragment();

    // Gửi ACK trạng thái phân mảnh (Fragment Status) qua UART (KHÔNG DÙNG SPI)
    if (g_flag_push_fragment)
    {
      g_flag_push_fragment = false;

      frame_work_t frame_push;
      memset(&frame_push, 0, sizeof(frame_push));

      frame_push.header.type = read_ram;
      frame_push.header.msk_regs = FRAGMENT_STATUS_RES;

      memcpy(frame_push.data, (uint8_t *)&g_cached_fragment_status.status,
             sizeof(uint64_t));

      frame_push.header.check_crc =
          Handle_SPIs.calcCRC(frame_push.data, frame_push.header.msk_regs.len);

      Serial1.write((uint8_t *)&frame_push,
                    sizeof(frame_push.header) + frame_push.header.msk_regs.len);
      // dbg_main("Push Fragment Status via UART: %08X",
      // (uint32_t)g_cached_fragment_status.status);
    }

    // Process request data push (CMD 97 - Tag -> Server)
    if (g_flag_push_request)
    {
      g_flag_push_request = false;
      frame_work_t frame_push;
      memset(&frame_push, 0, sizeof(frame_push));
      frame_push.header.type = read_ram;
      frame_push.header.msk_regs = REQUEST_DATA_RES;
      memcpy(frame_push.data, (uint8_t *)&g_cached_request_data,
             sizeof(request_data_t));
      frame_push.header.check_crc =
          Handle_SPIs.calcCRC(frame_push.data, frame_push.header.msk_regs.len);

      Serial1.write((uint8_t *)&frame_push,
                    sizeof(frame_push.header) + frame_push.header.msk_regs.len);
      dbg_main("Pushed REQUEST_DATA to U6 (len=%d)",
               frame_push.header.msk_regs.len);
    }

    // ==== BSS-TWR Result: Gửi qua SPI (pipeline TDOA) thay vì UART ====
    if (g_twr_q_head != g_twr_q_tail)
    { // Nếu Queue không trống
      dw_twr_result_t res;
      // Copy từ Queue ra
      memcpy(&res, &g_twr_queue[g_twr_q_head], sizeof(dw_twr_result_t));
      // Dịch Head lên
      g_twr_q_head = (g_twr_q_head + 1) % U7_TWR_QUEUE_SIZE;

      // Format: "T,<TagID_hex>,<Distance_mm>\r\n"
      // VD: "T,1026252682,29\r\n"
      char twr_buf[64];
      int twr_len =
          snprintf(twr_buf, sizeof(twr_buf), "T,%02X%02X%02X%02X%02X,%lu:\r\n",
                   res.tag_id[0], res.tag_id[1], res.tag_id[2], res.tag_id[3],
                   res.tag_id[4], res.distance_mm);

      // Đẩy vào SPI buffer (cùng pipeline với TDOA)
      Handle_Com.GiveBuff((uint8_t *)twr_buf, (uint8_t)twr_len);

      dbg_dw_twr("Tag=%02X%02X%02X%02X%02X Dist=%lu mm\n", res.tag_id[0],
                    res.tag_id[1], res.tag_id[2], res.tag_id[3], res.tag_id[4],
                    res.distance_mm);
    }

    if (FlagOTA == 0 && FlagISP3080OTA == 0) //
    {
      Taks_Spi_tx();
      // chương trình xử lý led trạng thái
    }

    if (checktimeOTA.ToEExpired())
    {
      dbg_main("timeout ota");
      FlagOTA = false;
      write_ota_t end_ota;
      frame_work_t frame_work;
      memset((uint8_t *)&end_ota, 0, sizeof(end_ota));
      end_ota.size_data = strlen("Update_fail");
      memcpy((uint8_t *)&end_ota.data, "Update_fail", end_ota.size_data);
      frame_work.header.msk_regs.len =
          end_ota.size_data + sizeof(end_ota.size_data);
      memcpy((uint8_t *)&frame_work.data, &end_ota,
             frame_work.header.msk_regs.len);
      frame_work.header.check_crc =
          Handle_SPIs.calcCRC(frame_work.data, frame_work.header.msk_regs.len);
      Serial1.write((uint8_t *)&frame_work,
                    sizeof(frame_work.header) + frame_work.header.msk_regs.len);
    }

    // if (ESPRebootTo.ToEExpired())
    // {
    // 	dbg_main("Esp Rebooting...");
    // 	// ESP.restart();
    // }

    delay(1);
  }
}

void codeForTaskDw1000Core1(void *parameter)
{
  while (1)
  {
    if (FlagOTA == 0 && FlagISP3080OTA == 0)
    {
      Handle_Dw.reloadDW();
      Handle_Dw.reciver();
      Handle_Dw.check_isp3080();
      Handle_Dw.handle_ducthang_twr();
      Handle_Dw.tdoa();
      // Handle_Dw.sync_tx();
      // Handle_Dw.sync_rx();
      Handle_Dw.tx_offset();
      Handle_Dw.offset_rx();
      Handle_Dw.twr();
      Handle_Dw.recheck();
      Handle_Dw.checkResetDW();
    }
    delay(1);
  }
}

/* Hàm init() được khởi tạo sẵn trong core esp32, từ đây ta tạo 3 task chính để
 * chạy thay vì trong hàm setup() */
// void init(void)
// {
// }

void setup()
{
  delay(10);
  Init_Setup();

  /* Task main */ // 10 * 1024
  xTaskCreatePinnedToCore(codeForTaskMainCore0, "TaskMainCore0", 10 * 1024,
                          NULL, 1, &MainCore0, 0);
  /* Task chay dw1000 */
  xTaskCreatePinnedToCore(codeForTaskDw1000Core1, "TaskDw1000Core1", 15 * 1024,
                          NULL,
                          2, // configMAX_PRIORITIES - 1,
                          &Dw1000Core1, 1);
}

void loop() {}