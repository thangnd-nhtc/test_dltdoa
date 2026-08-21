#include "handle_ISP3080.h"
#include "freertos/FreeRTOS.h" // Added by user instruction
#include "freertos/semphr.h"
#include "freertos/semphr.h" // Added by user instruction (already present, but duplicated in instruction)
#include "handle_decawave.h"
#include <Arduino.h>
#include <SPI.h>

// PTB
server_dataframe_t server_frame_rx;
dw_dataframe_t frame_twr;
dw_dataframe_t frame_ofsset_rx;
bool final_twr = false;
uint64_t g_fragment_status_rx = 0;
request_data_t g_request_data_rx;
// PTB check ack isp3080
uint32_t last_quiet_ms = 0; // định nghĩa thật
bool flag_isp3080 = false;
bool is_sending_config = false;  // Mặc định không bận
uint8_t temp_id_mode = 0;        // PTB: Luu mode hien tai cua ISP3080
bool g_bcast_twr_active = false; // Co luu trang thai BSS-TWR

// CHECK ACK ISP3080
// void check_ack_isp3080(void)
// {
//     uint32_t now = millis();

//     // Nếu module đang hoạt động (flag được set true ở nơi khác)
//     if (flag_isp3080)
//     {
//         // cập nhật thời gian hoạt động cuối cùng
//         last_quiet_ms = now;
//         flag_isp3080 = false; // reset cờ để lần sau đợi tín hiệu mới
//         return;
//     }

//     // Nếu không có tín hiệu trong 500ms liên tục
//     if ((uint32_t)(now - last_quiet_ms) > QUIET_MS)
//     {
//         Serial.println("ISP3080 khong co goi tin trong 500ms!");
//         uint8_t frame_mode = read_frame_from_nrf();
//         //dbg_dw("⚠️ ISP3080 không phản hồi trong 500ms!");
//         // có thể thêm hành động khác ở đây, ví dụ:
//         // reset_isp3080();
//         last_quiet_ms = now; // tránh log lặp
//     }
// }
// void check_ack_isp3080(void)
// {
//     uint32_t now = millis();

//     // Có gói tin/hoạt động gần đây → cập nhật mốc thời gian
//     if (flag_isp3080)
//     {
//         last_quiet_ms = now;
//         flag_isp3080 = false; // chờ sự kiện mới lần sau
//         return;
//     }

//     // Chưa vượt ngưỡng 500ms im lặng
//     if ((uint32_t)(now - last_quiet_ms) <= QUIET_MS)
//         return;

//     // Đã im lặng > 500ms → kiểm tra “còn sống” qua SPI (3 lần)
//     bool alive = false;
//     for (uint8_t i = 0; i < 3; ++i)
//     {
//         uint8_t rx_buf[BUF_LEN] = {0};
//         bool ok = send_cmd_to_isp3080(0x00, NULL, 0, rx_buf, sizeof(rx_buf),
//         100); if (ok)
//         {
//             // Có phản hồi SPI → coi như còn sống
//             alive = true;
//             last_quiet_ms = millis(); // reset mốc để tránh log lặp
//             Serial.println("ISP3080 alive via SPI check");
//             break;
//         }
//         delay(20); // nghỉ ngắn giữa các lần thử
//     }

//     if (!alive)
//     {
//         Serial.println("ISP3080 treo: khong phan hoi sau 3 lan SPI check!");
//         // Nếu muốn tự động reset, bỏ comment:
//         // reset_isp3080();
//         // last_quiet_ms = millis(); // tránh spam log sau khi reset
//     }
// }

// void check_ack_isp3080(void)
// {
//     uint32_t now = millis();

//     // Có gói tin/hoạt động gần đây → cập nhật mốc thời gian
//     if (flag_isp3080)
//     {
//         last_quiet_ms = now;
//         flag_isp3080 = false; // chờ sự kiện mới lần sau
//         return;
//     }

//     // Chưa vượt ngưỡng 500ms im lặng
//     if ((uint32_t)(now - last_quiet_ms) <= QUIET_MS)
//         return;

//     // Đã im lặng > 500ms → kiểm tra “còn sống” qua SPI (3 lần)
//     bool alive = false;
//     for (uint8_t i = 0; i < 3; ++i)
//     {
//         uint8_t rx_buf[BUF_LEN] = {0};
//         bool ok = send_cmd_to_isp3080(0x00, NULL, 0, rx_buf, sizeof(rx_buf),
//         100); if (ok)
//         {
//             alive = true;
//             last_quiet_ms = millis(); // reset mốc để tránh log lặp

//             // ✅ In log chi tiết phản hồi SPI
//             uint8_t cmd = rx_buf[0];
//             uint8_t status = rx_buf[1];
//             uint8_t frame_type = rx_buf[2];
//             uint8_t frame_len = rx_buf[3];

//             Serial.println("ISP3080 alive via SPI check");
//             Serial.print("  CMD: 0x");
//             Serial.println(cmd, HEX);
//             Serial.print("  STATUS: 0x");
//             Serial.println(status, HEX);
//             Serial.print("  FRAME_TYPE: 0x");
//             Serial.println(frame_type, HEX);
//             Serial.print("  FRAME_LEN: ");
//             Serial.println(frame_len);
//             break;
//         }
//         delay(20); // nghỉ ngắn giữa các lần thử
//     }

//     if (!alive)
//     {
//         Serial.println("ISP3080 treo: khong phan hoi sau 3 lan SPI check!");
//     }
// }
bool check_ack_isp3080(void)
{
  uint32_t now = millis();

  // Có gói tin/hoạt động gần đây → cập nhật mốc thời gian
  if (flag_isp3080)
  {
    last_quiet_ms = now;
    flag_isp3080 = false; // chờ sự kiện mới lần sau
    return true;          // vẫn coi như "alive"
  }

  // Chưa vượt ngưỡng 500ms im lặng
  if ((uint32_t)(now - last_quiet_ms) <= QUIET_MS)
    return true; // chưa cần kiểm tra, tạm coi còn sống

  // Đã im lặng > 500ms → kiểm tra “còn sống” qua SPI (3 lần)
  bool alive = false;
  for (uint8_t i = 0; i < MAX_TRY; ++i)
  {
    uint8_t rx_buf[BUF_LEN] = {0};
    bool ok = send_cmd_to_isp3080(0x00, NULL, 0, rx_buf, sizeof(rx_buf), 100);
    if (ok)
    {
      // Kiểm tra phản hồi có hợp lệ hay không (loại 0xFF FF FF FF)
      if (!(rx_buf[0] == 0xFF && rx_buf[1] == 0xFF && rx_buf[2] == 0xFF &&
            rx_buf[3] == 0xFF))
      {
        alive = true;
        last_quiet_ms = millis(); // reset mốc để tránh log lặp

        // ✅ In log chi tiết phản hồi SPI
        // uint8_t cmd = rx_buf[0];
        // uint8_t status = rx_buf[1];
        // uint8_t frame_type = rx_buf[2];
        // uint8_t frame_len = rx_buf[3];

        // Serial.println("ISP3080 alive via SPI check");
        //  Serial.printf("  CMD: 0x%02X\n", cmd);
        //  Serial.printf("  STATUS: 0x%02X\n", status);
        //  Serial.printf("  FRAME_TYPE: 0x%02X\n", frame_type);
        //  Serial.printf("  FRAME_LEN: %u\n", frame_len);
        break;
      }
      else
      {
        dbg_spi("%s", "ISP3080 phản hồi toàn 0xFF → không hợp lệ");
      }
    }

    delay(RETRY_GAP); // nghỉ ngắn giữa các lần thử
  }

  if (!alive)
  {
    dbg_spi("%s", "ISP3080 treo: không phản hồi sau 3 lần SPI check!");
  }

  return alive;
}

// PTB

/* === SPI bằng HSPI === */
static SPIClass isp3080_spi(HSPI); // <-- dùng HSPI

static SPISettings isp3080_settings;
// SemaphoreHandle_t nrf_spi_mutex = NULL; // BỎ MUTEX THEO YÊU CẦU USER

void reset_isp3080()
{
  if (temp_id_mode == 5)
  {
    dbg_spi("%s", ">>> Skip reset ISP3080 (Mode 5 active) <<<");
    return;
  }
  // Bỏ qua reset phần cứng nếu OTA đang diễn ra (do lock is_sending_config)
  if (is_sending_config)
  {
    dbg_spi("%s", ">>> Skip reset ISP3080 (OTA in progress) <<<");
    return;
  }
  pinMode(rst_isp3080, OUTPUT);
  digitalWrite(rst_isp3080, LOW);
  delay(10);
  digitalWrite(rst_isp3080, HIGH);
  delay(10);
}

void isp3080_spi_init(uint32_t freq_hz)
{
  // if (nrf_spi_mutex == NULL) {
  //   nrf_spi_mutex = xSemaphoreCreateMutex();
  // }

  pinMode(ISP3080_SPI_PIN_CS, OUTPUT);
  digitalWrite(ISP3080_SPI_PIN_CS, HIGH);

  // HSPI.begin(SCK, MISO, MOSI, SS)
  isp3080_spi.begin(ISP3080_SPI_PIN_SCK, ISP3080_SPI_PIN_MISO,
                    ISP3080_SPI_PIN_MOSI, ISP3080_SPI_PIN_CS);

  isp3080_settings = SPISettings(freq_hz, MSBFIRST, SPI_MODE0);
}

void isp3080_spi_cs_assert(void) { digitalWrite(ISP3080_SPI_PIN_CS, LOW); }
void isp3080_spi_cs_release(void) { digitalWrite(ISP3080_SPI_PIN_CS, HIGH); }

uint8_t isp3080_spi_txrx8(uint8_t tx)
{
  isp3080_spi.beginTransaction(isp3080_settings);
  // Thêm delay trước khi assert CS
  isp3080_spi_cs_release(); // Nhả CSN lên HIGH
  delayMicroseconds(10);    // Cho SPIS có thời gian chuẩn bị

  isp3080_spi_cs_assert();
  uint8_t rx = isp3080_spi.transfer(tx);
  isp3080_spi_cs_release();
  isp3080_spi.endTransaction();
  return rx;
}

void isp3080_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
  // BỎ MUTEX THEO YÊU CẦU USER ĐỂ TĂNG TỐC ĐỘ
  isp3080_spi.beginTransaction(isp3080_settings);

  // Dead-time ổn định chân CS
  isp3080_spi_cs_release();
  delayMicroseconds(10);

  isp3080_spi_cs_assert();
  isp3080_spi.transferBytes(const_cast<uint8_t *>(tx), rx, len);
  isp3080_spi_cs_release();

  isp3080_spi.endTransaction();
}

void isp3080_spi_write(const uint8_t *buf, size_t len)
{
  isp3080_spi_transfer(buf, nullptr, len);
}

void isp3080_spi_read(uint8_t *buf, size_t len)
{
  isp3080_spi_transfer(nullptr, buf, len);
}

bool send_cmd_to_isp3080(uint8_t cmd, const uint8_t *payload,
                         size_t payload_len, uint8_t *response,
                         size_t response_len, uint32_t wait_bus_ms)
{
  // Không dùng mutex nữa (theo yêu cầu user)

  const size_t max_buf_len = BUF_LEN;
  size_t tx_len = 1 + payload_len;
  size_t total_len = max(tx_len, response_len);

  if (total_len > max_buf_len)
  {
    return false;
  }

  uint8_t tx_buf[max_buf_len];
  uint8_t rx_buf[max_buf_len];

  memset(tx_buf, 0xFF, max_buf_len);
  memset(rx_buf, 0x00, max_buf_len);

  tx_buf[0] = cmd;
  if (payload && payload_len)
  {
    memcpy(&tx_buf[1], payload, payload_len);
  }

  // Truyền SPI
  isp3080_spi_transfer(tx_buf, rx_buf, total_len);

  bool success = false;
  // Kiểm tra phản hồi có dữ liệu thực (không phải toàn 0xFF)
  if (!(rx_buf[0] == 0xFF && rx_buf[1] == 0xFF && rx_buf[2] == 0xFF &&
        rx_buf[3] == 0xFF))
  {
    if (response && response_len)
    {
      memcpy(response, rx_buf, response_len);
    }
    success = true;
    last_quiet_ms =
        millis(); // Cập nhật mốc thời gian khi có phản hồi SPI hợp lệ
  }

  return success;
}

bool get_dw_frame_from_nrf(dw_dataframe_t *out_frame)
{
  if (out_frame == nullptr)
    return false;

  const size_t rx_len = sizeof(dw_dataframe_t) + 2;
  uint8_t rx_buf[rx_len] = {0};

  bool ok = send_cmd_to_isp3080(CMD_GET_FRAME_DW, nullptr, 0, rx_buf, rx_len);
  if (!ok)
  {
    dbg_spi("%s", "❌ Không gửi được CMD_GET_DW_FRAME.");
    return false;
  }

  // Kiểm tra phản hồi hợp lệ
  if (rx_buf[0] != (CMD_GET_FRAME_DW | 0x80))
  {
    //("⚠️ Phản hồi sai CMD (0x%02X)\n", rx_buf[0]);
    return false;
  }

  if (rx_buf[1] != 0x00)
  {
    // Serial.printf("❌ Gói phản hồi CMD_GET_DW_FRAME lỗi: mã lỗi = 0x%02X\n",
    // rx_buf[1]);
    return false;
  }

  // Debug raw frame nếu cần
  // Serial.print("Raw DW Frame: ");
  // for (int i = 2; i < rx_len; i++)
  //     Serial.printf("%02X ", rx_buf[i]);
  // Serial.println();

  memcpy(out_frame, &rx_buf[2], sizeof(dw_dataframe_t));
  return true;
}

bool get_server_frame_from_nrf(server_dataframe_t *out_frame)
{
  if (out_frame == nullptr)
    return false;

  const size_t rx_len = sizeof(server_dataframe_t) + 2;
  uint8_t rx_buf[rx_len] = {0};

  // Gửi lệnh CMD_GET_FRAME đến nRF và nhận phản hồi vào rx_buf
  bool ok =
      send_cmd_to_isp3080(CMD_GET_FRAME_SERVER, nullptr, 0, rx_buf, rx_len);
  if (!ok)
  {
    dbg_spi("%s", "❌ Không gửi được CMD_GET_FRAME.");
    return false;
  }

  // Kiểm tra byte lệnh phản hồi có đúng không
  if (rx_buf[0] != (CMD_GET_FRAME_SERVER | 0x80))
  {
    // Serial.printf("⚠️ Phản hồi sai CMD (0x%02X)\n", rx_buf[0]);
    return false;
  }

  // Kiểm tra mã lỗi (byte thứ 2)
  if (rx_buf[1] != 0x00)
  {
    // Serial.printf("❌ Gói phản hồi CMD_GET_FRAME lỗi: mã lỗi = 0x%02X\n",
    // rx_buf[1]);
    return false;
  }

  // Sao chép dữ liệu từ phần payload
  memcpy(out_frame, &rx_buf[2], sizeof(server_dataframe_t));
  return true;
}

void print_server_dataframe(const server_dataframe_t *f)
{
  // printf("\n[SERVER_FRAME] ==============================\n");
  // printf("Type_data : %u\n", f->Type_data);

  // uint32_t packet_id = (f->Packit_ID[3] << 24) | (f->Packit_ID[2] << 16) |
  //                      (f->Packit_ID[1] << 8) | f->Packit_ID[0];
  // printf("Packet_ID : %u\n", packet_id);

  // uint32_t serial_id = (f->Serial_ID[3] << 24) | (f->Serial_ID[2] << 16) |
  //                      (f->Serial_ID[1] << 8) | f->Serial_ID[0];
  // printf("Serial_ID : %u\n", serial_id);

  // uint64_t timestamp = 0;
  // for (int i = 4; i >= 0; i--)
  //     timestamp = (timestamp << 8) | f->Timestamp[i];
  // printf("Timestamp : %llu\n", timestamp);

  // uint32_t tag_id = (f->Tag_ID[3] << 24) | (f->Tag_ID[2] << 16) |
  //                   (f->Tag_ID[1] << 8) | f->Tag_ID[0];
  // printf("Tag_ID    : %u\n", tag_id);

  // printf("Motion    : %u\n", f->Motion);
  // printf("Button    : %u\n", f->Button);
  // printf("Free_fall : %u\n", f->Free_fall);
  // printf("RSSI      : %u\n", f->RSSI);

  // switch (f->Type_data)
  // {
  // case Cmd_tag_sensor:
  //     printf("[SENSOR] Compass     : %u\n", *(uint32_t *)f->Type1.Compass);
  //     printf("         Pressure    : %u\n", *(uint32_t *)f->Type1.Pressure);
  //     printf("         Accelerome  : %u\n", *(uint32_t
  //     *)f->Type1.Accelermeter); break;

  // case Cmd_tag_solut:
  //     printf("[SOLUT ] Temp        : %u\n", f->Type2.Temperature);
  //     printf("         Humi        : %u\n", f->Type2.Humidity);
  //     printf("         Vibra       : %u\n", f->Type2.Vibrate);
  //     break;

  // case Cmd_tag_dps422:
  //     printf("[DPS422] Temp        : %u\n", (f->Type3.Temperature[1] << 8) |
  //     f->Type3.Temperature[0]); printf("         Pressure    : %u\n",
  //     *(uint32_t *)f->Type3.Pressure); break;

  // default:
  //     printf("[INFO] No extended data.\n");
  //     break;
  // }

  // In dữ liệu raw hex:
  // printf("[RAW DATA HEX]: ");
  // const uint8_t *raw = (const uint8_t *)f;
  // for (size_t i = 0; i < sizeof(server_dataframe_t); i++)
  // {
  //     printf("%02X ", raw[i]);
  //     if ((i + 1) % 16 == 0)
  //         printf("\n                ");
  // }
  // printf("\n");

  // printf("=============================================\n");
}

void print_dw_dataframe(const dw_dataframe_t *frame)
{
  // printf("\n[DW_FRAME] ==============================\n");
  // printf("Des      : %u\n", frame->Des);
  // printf("Src      : %u\n", frame->Src);
  // printf("PacketId : %u\n", frame->Packet_Id);
  // printf("Cmd      : %u\n", frame->Cmd);
  // printf("TypeDev  : %u\n", frame->TypeDev);

  // switch (frame->Cmd)
  // {
  // case Cmd_Sync:
  //     printf("[SYNC] Ts: ");
  //     for (int i = 0; i < 5; i++)
  //         printf("%02X ", frame->Data.SYNC.Ts[i]);
  //     printf("\n");
  //     break;

  // case Cmd_Distance:
  //     printf("[DIST] Distance: %u\n", frame->Data.DIST.Dis);
  //     break;

  // case Cmd_Pool:
  // case Cmd_Resp:
  // case Cmd_Final:
  //     printf("[DS_TWR]\n");
  //     printf("  poll_tx : ");
  //     for (int i = 0; i < 5; i++)
  //         printf("%02X ", frame->Data.DS_TWR.TIME_STAMP.poll_tx[i]);
  //     printf("\n  resp_rx : ");
  //     for (int i = 0; i < 5; i++)
  //         printf("%02X ", frame->Data.DS_TWR.TIME_STAMP.resp_rx[i]);
  //     printf("\n  final_tx: ");
  //     for (int i = 0; i < 5; i++)
  //         printf("%02X ", frame->Data.DS_TWR.TIME_STAMP.final_tx[i]);
  //     printf("\n");
  //     break;

  // case Cmd_Tag:
  //     printf("[TAG] Type=%u, Batt=%u, Motion=%u, Button=%u\n",
  //            frame->Data.TAG.Type,
  //            frame->Data.TAG.Battery,
  //            frame->Data.TAG.Motion,
  //            frame->Data.TAG.Button);

  //     switch (frame->Data.TAG.Type)
  //     {
  //     case Cmd_tag_sensor:
  //         printf("  [SENSOR] Compass   : %u\n",
  //         frame->Data.TAG.Custom.ETAG.Compass); printf("           Pressure
  //         : %u\n", frame->Data.TAG.Custom.ETAG.Pressure); printf(" Accel :
  //         %u\n", frame->Data.TAG.Custom.ETAG.Acceleration); break;

  //     case Cmd_tag_solut:
  //         printf("  [SOLUT ] Temp      : %u\n",
  //         frame->Data.TAG.Custom.SOLUT.Temper); printf("           Humi :
  //         %u\n", frame->Data.TAG.Custom.SOLUT.Humi); printf("           Vibra
  //         : %u\n", frame->Data.TAG.Custom.SOLUT.Vibra); break;

  //     case Cmd_tag_dps422:
  //         printf("  [DPS422] Temp      : %u\n",
  //         frame->Data.TAG.Custom.DPS422.Temper); printf("           Pressure
  //         : %u\n", frame->Data.TAG.Custom.DPS422.Pressure); break;

  //     default:
  //         printf("  [TAG] Unknown Subtype: %u\n", frame->Data.TAG.Type);
  //         break;
  //     }
  //     break;

  // default:
  //     printf("[UNKNOWN] Khong xac dinh loai goi tin (Cmd=0x%02X)\n",
  //     frame->Cmd); break;
  // }

  // printf("CRC      : 0x%04X\n", frame->DCRC);
  // printf("=========================================\n");
}

#include "handle_com.h"
#include "handle_spis.h"
#include <TimeOutEvent.h>

extern TimeOutEvent fragPollTimer;
extern bool is_polling_fragment;
extern uint64_t last_fragment_status;

uint8_t read_frame_from_nrf()
{
  if (is_sending_config)
    return false; // Tránh tranh chấp khi đang sync config

  uint8_t rx_buf[BUF_LEN] = {0}; // đủ dài để chứa toàn bộ frame
  uint8_t cmd_to_send = 0x00;

  /*
    if (is_polling_fragment && fragPollTimer.ToEExpired()) {
      cmd_to_send = CMD_GET_FRAGMENT_STATUS;
      fragPollTimer.ToEUpdate(2000);
      // Serial.println("\r\n[U7] fragPollTimer tick: Querying "
      //                "CMD_GET_FRAGMENT_STATUS in SPI loop...");
    }
  */

  // Task đọc định kỳ: Chỉ đợi bus 10ms. Nếu bận quá thì bỏ qua chu kỳ này.
  if (send_cmd_to_isp3080(cmd_to_send, NULL, 0, rx_buf, sizeof(rx_buf), 100))
  {
    uint8_t cmd = rx_buf[0];
    uint8_t status = rx_buf[1];
    uint8_t frame_type = rx_buf[2];
    uint8_t frame_len = rx_buf[3];

    /*
        // Bắt nhanh gói trả lời CMD_GET_FRAGMENT_STATUS
        // Nếu lần đầu không nhận được response đúng, retry 1 lần với delay nhỏ
        if (cmd_to_send == CMD_GET_FRAGMENT_STATUS &&
            !(cmd == (CMD_GET_FRAGMENT_STATUS | 0x80) && status == 0x00 &&
              frame_len == 8)) {
          if (send_cmd_to_isp3080(0x00, NULL, 0, rx_buf, sizeof(rx_buf), 50)) {
            cmd = rx_buf[0];
            status = rx_buf[1];
            frame_type = rx_buf[2];
            frame_len = rx_buf[3];
          }
        }

        if (cmd == (CMD_GET_FRAGMENT_STATUS | 0x80) && status == 0x00 &&
            frame_len == 8) {
          uint64_t new_status;
          memcpy(&new_status, &rx_buf[4], 8);

          if (new_status != last_fragment_status || (new_status & (1ULL << 63)))
       { last_fragment_status = new_status; extern volatile bool
       g_flag_push_fragment; extern fragment_status_t g_cached_fragment_status;

            g_cached_fragment_status.status = new_status;
            g_flag_push_fragment = true;
          }

          if (new_status & (1ULL << 63)) {
            is_polling_fragment = false;
          }
          return false;
        }
    */

    if (status == 0x00 && frame_len <= (BUF_LEN - 4))
    {
      void *payload = &rx_buf[4];

      switch (frame_type)
      {
      case SPI_FRAME_TYPE_SERVER:
        //  print_server_dataframe((server_dataframe_t *)payload);
        //  Sao chép payload vào biến server_frame
        memcpy(&server_frame_rx, payload, sizeof(server_dataframe_t));
        return SPI_FRAME_TYPE_SERVER;

      case SPI_FRAME_TYPE_DW:
        memcpy(&frame_twr, payload, sizeof(dw_dataframe_t));
        final_twr = true;
        // Serial.printf("[U7] SPI_RX: FRAME_TYPE_DW (0x01). Cmd = 0x%02X\n",
        //               frame_twr.Cmd);
        return SPI_FRAME_TYPE_DW;

      case SPI_FRAME_TYPE_BCAST_TWR_RESULT:
      {
        dw_twr_result_t twr_res;
        memcpy(&twr_res, payload, sizeof(dw_twr_result_t));

        // Đẩy kết quả vào hàng đợi (Queue) an toàn đa luồng
        extern dw_twr_result_t g_twr_queue[];
        extern volatile int g_twr_q_head;
        extern volatile int g_twr_q_tail;
        // Kích thước Queue bằng 32 đã được cấp phát bên main.cpp
        int next_tail = (g_twr_q_tail + 1) % 32;
        if (next_tail != g_twr_q_head)
        { // Nếu Queue chưa đầy
          memcpy(&g_twr_queue[g_twr_q_tail], &twr_res, sizeof(dw_twr_result_t));
          g_twr_q_tail = next_tail;
        }
        else
        {
          dbg_spi("%s",
              "[U7-Core1] CẢNH BÁO: TWR Queue bị ĐẦY! Đang rớt gói!");
        }
        return SPI_FRAME_TYPE_BCAST_TWR_RESULT;
      }

      case SPI_FRAME_TYPE_OFFSET:
        // handle_offset_rx((dw_dataframe_t *)payload);
        memcpy(&frame_ofsset_rx, payload, sizeof(dw_dataframe_t));
        print_dw_dataframe((dw_dataframe_t *)payload); // frame_ofsset_rx
        return SPI_FRAME_TYPE_OFFSET;

      case SPI_FRAME_TYPE_REQUEST:
        memcpy(&g_request_data_rx, payload, sizeof(request_data_t));
        dbg_spi("%s", "\r\n[U7] >>> Received Request Data from Tag via SPI");
        print_request_data(&g_request_data_rx);

        // PTB: Đẩy sang U6 để gửi lên server
        {
          extern volatile bool g_flag_push_request;
          extern request_data_t g_cached_request_data;
          memcpy(&g_cached_request_data, &g_request_data_rx,
                 sizeof(request_data_t));
          g_flag_push_request = true;
        }
        return SPI_FRAME_TYPE_REQUEST;

      case SPI_FRAME_TYPE_ACK_STATUS:
        memcpy(&g_fragment_status_rx, payload, sizeof(uint64_t));

        // Tự động đẩy thông tin sang U6 (Core 0 xử lý Serial1) khi có cập nhật
        if (g_fragment_status_rx != last_fragment_status ||
            (g_fragment_status_rx & (1ULL << 63)))
        {
          last_fragment_status = g_fragment_status_rx;

          extern volatile bool g_flag_push_fragment;
          extern fragment_status_t g_cached_fragment_status;

          g_cached_fragment_status.status = g_fragment_status_rx;
          g_flag_push_fragment = true;
        }

        if (g_fragment_status_rx & (1ULL << 63))
        {
          is_polling_fragment = false;
        }

        return SPI_FRAME_TYPE_ACK_STATUS;

      default:
        // Serial.printf("⚠️ Unknown frame_type: %02X\n", frame_type);
        return false;
      }
    }
    else
    {
      // Serial.printf("❌ Lỗi phản hồi: status=%02X, len=%d\n", status,
      // frame_len);
      return false;
    }
  }
  else
  {
    dbg_spi("%s", "❌ SPI giao tiếp thất bại");
    return false;
  }
}

bool begin_dw3000()
{
  const int retry_limit = 3;
  uint8_t resp[2] = {0};

  for (int attempt = 1; attempt <= retry_limit; attempt++)
  {
    // Serial.printf("🌀 [BEGIN_DW3000] Thử lần %d...\n", attempt);

    if (send_cmd_to_isp3080(CMD_BEGIN_DW3000, nullptr, 0, resp, sizeof(resp)))
    {
      if (resp[1] == 0x00 && resp[0] != 0x00)
      {
        dbg_spi("%s", "✅ DW3000 khởi tạo thành công!");
        return true;
      }
      else
      {
        // Serial.printf("❌ Lỗi khởi tạo DW3000, mã lỗi = 0x%02X\n", resp[1]);
      }
    }
    else
    {
      dbg_spi("%s", "❌ Giao tiếp SPI thất bại khi gửi CMD_BEGIN_DW3000");
    }
    delay(10); // đợi 20ms trước khi thử lại
  }

  dbg_spi("%s", "🛑 Khởi tạo DW3000 thất bại");
  return false;
}

// bool enable_rx_mode()
// {
//     uint8_t resp[2] = {0};
//     bool success = false;

//     if (send_cmd_to_isp3080(CMD_ENABLE_RX, nullptr, 0, resp, sizeof(resp)))
//     {
//         if (resp[1] == 0x00)
//         {
//             Serial.println("✅ Đã bật chế độ RX.");
//             success = true;
//         }
//         else
//         {
//             Serial.printf("❌ Lỗi bật RX, mã = 0x%02X\n", resp[1]);
//         }
//     }
//     else
//     {
//         Serial.println("❌ Gửi lệnh CMD_ENABLE_RX thất bại.");
//     }

//     return success;
// }
bool enable_rx_mode()
{
  const int max_retry = 3;
  uint8_t resp[2] = {0};

  for (int attempt = 1; attempt <= max_retry; attempt++)
  {
    // Serial.printf("🔁 [ENABLE_RX] Thử lần %d...\n", attempt);

    if (send_cmd_to_isp3080(CMD_ENABLE_RX, nullptr, 0, resp, sizeof(resp)))
    {
      if (resp[1] == 0x00)
      {
        // Serial.println("✅ Đã bật chế độ RX.");
        return true;
      }
      else
      {
        // Serial.printf("❌ Lỗi bật RX, mã = 0x%02X\n", resp[1]);
      }
    }
    else
    {
      // Serial.println("❌ Gửi lệnh CMD_ENABLE_RX thất bại.");
    }

    delay(10); // chờ trước khi thử lại
  }

  dbg_spi("%s", "❌ Bật chế độ RX thất bại sau 3 lần thử.");
  return false;
}

// bool set_device_id_to_nrf(uint32_t device_id)
// {
//     uint8_t resp[2] = {0};
//     if (send_cmd_to_isp3080(CMD_SET_DEVICE_ID, (uint8_t *)&device_id,
//     sizeof(device_id), resp, sizeof(resp)))
//     {
//         if (resp[1] == 0x00)
//         {
//             // Serial.printf("✅ Đã gán MY_DEVICE_ID = 0x%08lX cho nRF thành
//             công\n", device_id); Serial.printf("✅ Đã gán MY_DEVICE_ID = %lu
//             (0x%08lX) cho nRF thành công\n", device_id, device_id); return
//             true;
//         }
//         else
//         {
//             Serial.printf("❌ Lỗi gán ID cho nRF, mã lỗi = 0x%02X\n",
//             resp[1]);
//         }
//     }
//     else
//     {
//         Serial.println("❌ Gửi CMD_SET_DEVICE_ID thất bại");
//     }
//     return false;
// }
bool set_device_id_to_nrf(uint32_t device_id)
{
  const int max_retry = 3;
  uint8_t resp[2] = {0};

  for (int attempt = 1; attempt <= max_retry; attempt++)
  {
    // Serial.printf("🔁 [SET_DEVICE_ID] Thử lần %d...\n", attempt);

    if (send_cmd_to_isp3080(CMD_SET_DEVICE_ID, (uint8_t *)&device_id,
                            sizeof(device_id), resp, sizeof(resp)))
    {
      if (resp[1] == 0x00)
      {
        // Serial.printf("✅ Đã gán MY_DEVICE_ID = %lu (0x%08lX) cho nRF thành
        // công\n", device_id, device_id);
        return true;
      }
      else
      {
        // Serial.printf("❌ Lỗi gán ID cho nRF, mã lỗi = 0x%02X\n", resp[1]);
      }
    }
    else
    {
      dbg_spi("%s", "❌ Gửi CMD_SET_DEVICE_ID thất bại");
    }

    // delay(20); // đợi một chút trước khi thử lại
  }

  dbg_spi("%s", "❌ Gán MY_DEVICE_ID cho nRF thất bại sau 3 lần thử");
  return false;
}

// int build_csv_from_server_frame(const server_dataframe_t *frame, char
// *buffer, size_t bufsize)
// {
//     if (!frame || !buffer || bufsize == 0)
//         return 0;

//     // Chuyển đổi dữ liệu cơ bản
//     uint32_t packet_id = *((uint32_t *)frame->Packit_ID);
//     uint32_t serial_id = *((uint32_t *)frame->Serial_ID);
//     uint64_t timestamp = 0;
//     // memcpy(&timestamp, frame->Timestamp, 5); // lấy 5 byte thấp
//     timestamp = parse_ts_40bit(frame->Timestamp);

//     uint32_t tag_id = *((uint32_t *)frame->Tag_ID);
//     uint8_t motion = frame->Motion;
//     uint8_t button = frame->Button;
//     uint8_t battery = frame->Free_fall;
//     uint8_t rssi = frame->RSSI;

//     // Ghi master access (4 master)
//     uint32_t s0 = *((uint32_t *)frame->Mts_access[0].Serial);
//     uint64_t t0 = *((uint64_t *)frame->Mts_access[0].Timestamp);
//     uint32_t s1 = *((uint32_t *)frame->Mts_access[1].Serial);
//     uint64_t t1 = *((uint64_t *)frame->Mts_access[1].Timestamp);
//     uint32_t s2 = *((uint32_t *)frame->Mts_access[2].Serial);
//     uint64_t t2 = *((uint64_t *)frame->Mts_access[2].Timestamp);
//     uint32_t s3 = *((uint32_t *)frame->Mts_access[3].Serial);
//     uint64_t t3 = *((uint64_t *)frame->Mts_access[3].Timestamp);

//     // In ra chuỗi CSV
//     int len = snprintf(buffer, bufsize,
//                        "%d,%lu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%d,%d,%d,%d:\r\n",
//                        frame->Type_data,
//                        packet_id,
//                        serial_id,
//                        timestamp,
//                        s0, t0,
//                        s1, t1,
//                        s2, t2,
//                        s3, t3,
//                        tag_id,
//                        motion, button, battery, rssi);

//     return len; // trả về số byte ghi vào buffer
// }

bool set_beacon_config_to_nrf(beacon_cfg_t *cfg)
{
  if (cfg == nullptr)
    return false;

  temp_id_mode = cfg->val_id_mode;                   // Cập nhật mode hiện tại
  g_bcast_twr_active = (cfg->enable_bcast_twr == 1); // Cập nhật cờ BSS-TWR
  is_sending_config = true;                          // Bắt đầu khóa Reset
  delay(500);

  uint8_t payload[BUF_LEN] = {0};
  // nRF expects count at index 7 (m_rx_copy[7]), so cmd + 6 bytes header
  // tx_buf[0] = 0x10
  // tx_buf[1..6] = header (6 bytes dummy)
  // tx_buf[7] = count (payload[6])
  // tx_buf[8..] = items (starting from payload[7])

  uint8_t count = 0;
  uint8_t *p = &payload[7]; // SỬA: Bắt đầu từ byte thứ 7 (tx_buf[8])

  auto add_item = [&](uint16_t id, uint32_t val)
  {
    p[0] = id & 0xFF;
    p[1] = (id >> 8) & 0xFF;
    p[2] = val & 0xFF;
    p[3] = (val >> 8) & 0xFF;
    p[4] = (val >> 16) & 0xFF;
    p[5] = (val >> 24) & 0xFF;
    p += 6;
    count++;
  };

  // Packing all fields (Giữ nguyên thứ tự cũ nhưng có lọc > 0)
  if (cfg->val_motion > 0)
    add_item(MAJOR_TIMER_SEND_TAG_MOTION, cfg->val_motion);
  if (cfg->val_stand > 0)
    add_item(MAJOR_TIMER_SEND_TAG_STAND, cfg->val_stand);
  if (cfg->val_sleep1 > 0)
    add_item(MAJOR_TIMER_SEND_TAG_SLEEP_MODE_1, cfg->val_sleep1);
  if (cfg->val_sleep2 > 0)
    add_item(MAJOR_TIMER_SEND_TAG_SLEEP_MODE_2, cfg->val_sleep2);
  if (cfg->val_sleep3 > 0)
    add_item(MAJOR_TIMER_SEND_TAG_SLEEP_MODE_3, cfg->val_sleep3);

  if (cfg->val_mode1 > 0)
    add_item(MAJOR_TIMER_SLEEP_MODE_1, cfg->val_mode1);
  if (cfg->val_mode2 > 0)
    add_item(MAJOR_TIMER_SLEEP_MODE_2, cfg->val_mode2);
  if (cfg->val_mode3 > 0)
    add_item(MAJOR_TIMER_SLEEP_MODE_3, cfg->val_mode3);

  if (cfg->val_batt_default > 0)
    add_item(MAJOR_BATT_UPDATE_DEFAULT, cfg->val_batt_default);
  if (cfg->val_batt_high > 0)
    add_item(MAJOR_BATT_UPDATE_HIGH, cfg->val_batt_high);
  if (cfg->val_batt_inc > 0)
    add_item(MAJOR_BATT_INCREASE, cfg->val_batt_inc);
  if (cfg->val_batt_dec > 0)
    add_item(MAJOR_BATT_DECREASE, cfg->val_batt_dec);

  if (cfg->uwb_chan > 0)
    add_item(MAJOR_CONFIG_UWB_CHAN, cfg->uwb_chan);
  if (cfg->uwb_plen > 0)
    add_item(MAJOR_CONFIG_UWB_PLEN, cfg->uwb_plen);
  if (cfg->uwb_pac > 0)
    add_item(MAJOR_CONFIG_UWB_PAC, cfg->uwb_pac);
  if (cfg->uwb_txcode > 0)
    add_item(MAJOR_CONFIG_UWB_TXCODE, cfg->uwb_txcode);
  if (cfg->uwb_rxcode > 0)
    add_item(MAJOR_CONFIG_UWB_RXCODE, cfg->uwb_rxcode);
  if (cfg->uwb_sfdtype > 0)
    add_item(MAJOR_CONFIG_UWB_SFDTYPE, cfg->uwb_sfdtype);
  if (cfg->uwb_datarate > 0)
    add_item(MAJOR_CONFIG_UWB_DATARATE, cfg->uwb_datarate);
  if (cfg->uwb_phrmode > 0)
    add_item(MAJOR_CONFIG_UWB_PHRMODE, cfg->uwb_phrmode);
  if (cfg->uwb_phrrate > 0)
    add_item(MAJOR_CONFIG_UWB_PHRRATE, cfg->uwb_phrrate);
  if (cfg->uwb_sfdto > 0)
    add_item(MAJOR_CONFIG_UWB_SFDTO, cfg->uwb_sfdto);
  if (cfg->uwb_stsmode > 0)
    add_item(MAJOR_CONFIG_UWB_STSMODE, cfg->uwb_stsmode);
  if (cfg->uwb_stslen > 0)
    add_item(MAJOR_CONFIG_UWB_STSLEN, cfg->uwb_stslen);
  if (cfg->uwb_pdoa > 0)
    add_item(MAJOR_CONFIG_UWB_PDOA, cfg->uwb_pdoa);

  // --- Mapping State/Mode (0x2000 - 0x2007) ---
  switch (cfg->val_id_mode)
  {
  case 0:
    add_item(MAJOR_STATE_DEFAULT, 1);
    break;
  case 1:
    add_item(MAJOR_STATE_TX, 1);
    break;
  case 2:
    add_item(MAJOR_STATE_OFF_UWB, 1);
    break;
  case 3:
    add_item(MAJOR_STATE_SOS, 1);
    break;
  case 4:
    add_item(MAJOR_STATE_IDENTIFY, 1);
    break;
  case 5:
    add_item(MAJOR_STATE_TWR, 1);
    break;
  case 6:
    add_item(MAJOR_STATE_RESET, 1);
    break;
  case 7:
    add_item(MAJOR_STATE_AIRPLAN, 1);
    break;
  case 8:
    add_item(MAJOR_STATE_MOTION, 1);
    break;
  case 0xFE:
    add_item(0x20FE, 1); // MAJOR_STATE_NO_CHANGE
    break;
  default:
    // add_item(MAJOR_STATE_DEFAULT, 1);
    break;
  }

  // --- Mapping Request (0x30) ---
  switch (cfg->val_request)
  {
  case 1:
    add_item(MAJOR_REQUEST_TIMER, 1);
    break;
  case 2:
    add_item(MAJOR_REQUEST_CONFIG, 1);
    break;
  case 3:
    add_item(MAJOR_REQUEST_STATE, 1);
    break;
  case 4:
    add_item(MAJOR_REQUEST_ID_TAG, 1);
    break;
  default:
    break;
  }

  // --- Luôn gửi ID Last (5 bytes) để nRF52 biết Tag nào đang được chọn ---
  bool has_id_last = (cfg->val_id_last[0] > 0 || cfg->val_id_last[1] > 0 ||
                      cfg->val_id_last[2] > 0 || cfg->val_id_last[3] > 0 ||
                      cfg->val_id_last[4] > 0);
  if (has_id_last)
  {
    add_item(MAJOR_ID_LAST_BYTE1, cfg->val_id_last[0]);
    add_item(MAJOR_ID_LAST_BYTE2, cfg->val_id_last[1]);
    add_item(MAJOR_ID_LAST_BYTE3, cfg->val_id_last[2]);
    add_item(MAJOR_ID_LAST_BYTE4, cfg->val_id_last[3]);
    add_item(MAJOR_ID_LAST_BYTE5, cfg->val_id_last[4]);
  }

  // --- Gửi MAC Address (5 bytes) ---
  bool has_mac = (cfg->mac_address[0] > 0 || cfg->mac_address[1] > 0 ||
                  cfg->mac_address[2] > 0 || cfg->mac_address[3] > 0 ||
                  cfg->mac_address[4] > 0);
  if (has_mac)
  {
    add_item(MAJOR_MAC_ADDRESS_BYTE1, cfg->mac_address[0]);
    add_item(MAJOR_MAC_ADDRESS_BYTE2, cfg->mac_address[1]);
    add_item(MAJOR_MAC_ADDRESS_BYTE3, cfg->mac_address[2]);
    add_item(MAJOR_MAC_ADDRESS_BYTE4, cfg->mac_address[3]);
    add_item(MAJOR_MAC_ADDRESS_BYTE5, cfg->mac_address[4]);
  }

  // --- Gửi ID New (5 bytes) nếu có yêu cầu thay đổi (Checkbox Change ID được
  // tích) ---
  if (cfg->val_id_change == 1)
  {
    add_item(MAJOR_ID_CHANGE_FLAG, 1);
    add_item(MAJOR_ID_CHANGE_BYTE1, cfg->val_id_new[0]);
    add_item(MAJOR_ID_CHANGE_BYTE2, cfg->val_id_new[1]);
    add_item(MAJOR_ID_CHANGE_BYTE3, cfg->val_id_new[2]);
    add_item(MAJOR_ID_CHANGE_BYTE4, cfg->val_id_new[3]);
    add_item(MAJOR_ID_CHANGE_BYTE5, cfg->val_id_new[4]);
  }

  // --- Gửi Flag apply_config_to_base ---
  if (cfg->apply_config_to_base == 1)
  {
    add_item(MAJOR_APPLY_CONFIG_TO_BASE, 1);
  }

  // --- Gửi Flag Broadcast ---
  if (cfg->broadcast == 1)
  {
    add_item(MAJOR_BROADCAST_FLAG, 1);
  }

  // --- Gửi Flag Enable Broadcast TWR ---
  if (cfg->enable_bcast_twr == 1)
  {
    add_item(MAJOR_ENABLE_BCAST_TWR, 1);
  }

  // --- Gửi Setting CHARGE_TX (0x40) ---
  // Gửi giá trị gốc (1=OFF, 2=ON) qua SPI, nRF52 tự map sang 0x00/0xFF khi phát
  // BLE
  if (cfg->charge_tx == 1 || cfg->charge_tx == 2)
  {
    add_item(MAJOR_SETTING_CHARGE_TX, cfg->charge_tx);
  }

  // --- Gửi Setting OTA (0x4001) ---
  // Gửi giá trị qua SPI, nRF52 tự map sang 0xFF khi phát BLE
  if (cfg->ota_enable == 1)
  {
    add_item(MAJOR_SETTING_OTA, cfg->ota_enable);
  }

  // --- Gửi Setting SYS_CONFIG_DEFAULT (0x4002) ---
  // Gửi giá trị gốc qua SPI, nRF52 tự map sang 0xFF khi phát BLE
  if (cfg->sys_config_default == 1)
  {
    add_item(MAJOR_SETTING_SYS_CONFIG_DEFAULT, cfg->sys_config_default);
  }

  // --- Gửi Setting SLEEP (0x4003) ---
  // Gửi giá trị gốc (1=DISABLE, 2=ENABLE) qua SPI, nRF52 tự map sang 0x00/0xFF khi phát BLE
  if (cfg->sleep_enable == 1 || cfg->sleep_enable == 2)
  {
    add_item(MAJOR_SETTING_SLEEP, cfg->sleep_enable);
  }

  // Set count byte at tx_buf[7]
  payload[6] = count;

  // Serial.printf("[U7] --- Sync Start: %u items, Payload: %u bytes ---\n", count,
  //               7 + (count * 6));

  uint8_t resp[32] = {0};
  // Lần 1: Gửi toàn bộ dữ liệu cấu hình. Đợi bus tới 100ms.
  bool result = send_cmd_to_isp3080(CMD_SET_BEACON_DYNAMIC, payload,
                                    7 + (count * 6), resp, 32, 100);

  // Quan trọng: Gửi xong thì gia hạn chặn Reset ISP trong vòng 60 giây
  last_quiet_ms = millis() + 60000;

  // Kiểm tra ACK ngay lập tức (thường sẽ fail vì nRF bận)
  if (result && resp[0] == (CMD_SET_BEACON_DYNAMIC | 0x80))
  {
    // Sync thành công bước đầu
  }
  else
  {
    // Serial.println("ISP3080 is processing items... Waiting before retry
    // read.");

    // Thử lại tối đa 3 lần, mỗi lần cách nhau 50ms
    result = false; // Reset result for retry loop
    for (int retry = 0; retry < 3; retry++)
    {
      // delay(50);
      // Gửi lệnh rỗng (0x00) để lấy buffer TX đang chờ từ nRF. Timeout 10ms.
      if (send_cmd_to_isp3080(0x00, NULL, 0, resp, 32, 100))
      {
        if (resp[0] == (CMD_SET_BEACON_DYNAMIC | 0x80))
        {
          // Serial.printf("ISP3080 Sync OK (Attempt %d)\n", retry + 1);
          result = true;
          // Gia hạn chặn Reset ISP thêm 60 giây khi retry thành công
          last_quiet_ms = millis() + 60000;
          break;
        }
      }
    }
  }

  // if (!result) {
  //   Serial.println("ISP3080 Sync failed.");
  // }

  is_sending_config = false;
  return result;
}

int build_csv_from_server_frame(const server_dataframe_t *frame, char *buffer,
                                size_t bufsize)
{
  if (!frame || !buffer || bufsize == 0)
    return 0;

  static frame_entry_t frame_history[MAX_FRAME_HISTORY] = {0};
  static int history_index = 0;

  // Lấy giá trị so sánh
  uint32_t packet_id = *((uint32_t *)frame->Packit_ID);
  uint32_t tag_id = *((uint32_t *)frame->Tag_ID);

  // Kiểm tra trùng gói
  for (int i = 0; i < MAX_FRAME_HISTORY; i++)
  {
    if (frame_history[i].packet_id == packet_id &&
        frame_history[i].tag_id == tag_id)
    {
      // Gói trùng → bỏ qua
      return 0;
    }
  }

  // Nếu không trùng → lưu vào buffer vòng
  frame_history[history_index].packet_id = packet_id;
  frame_history[history_index].tag_id = tag_id;
  history_index = (history_index + 1) % MAX_FRAME_HISTORY;

  // Lấy các trường dữ liệu
  uint32_t serial_id = *((uint32_t *)frame->Serial_ID);
  uint64_t timestamp = parse_ts_40bit(frame->Timestamp);
  uint8_t motion = frame->Motion;
  uint8_t button = frame->Button;
  uint8_t battery = frame->Free_fall;
  uint8_t rssi = frame->RSSI;

  // Ghi master access
  uint32_t s0 = *((uint32_t *)frame->Mts_access[0].Serial);
  uint64_t t0 = *((uint64_t *)frame->Mts_access[0].Timestamp);
  uint32_t s1 = *((uint32_t *)frame->Mts_access[1].Serial);
  uint64_t t1 = *((uint64_t *)frame->Mts_access[1].Timestamp);
  uint32_t s2 = *((uint32_t *)frame->Mts_access[2].Serial);
  uint64_t t2 = *((uint64_t *)frame->Mts_access[2].Timestamp);
  uint32_t s3 = *((uint32_t *)frame->Mts_access[3].Serial);
  uint64_t t3 = *((uint64_t *)frame->Mts_access[3].Timestamp);

  // Ghi ra CSV
  int len = 0;
  if (frame->Type_data == 0)
  {
    uint16_t version;
    uint16_t temp;
    uint32_t hi_mac;
    uint32_t low_mac;
    memcpy(&version, frame->Type5.Version, 2);
    memcpy(&temp, frame->Type5.Temp, 2);
    memcpy(&hi_mac, frame->Type5.Hi_MAC, 4);
    memcpy(&low_mac, frame->Type5.Low_MAC, 4);

    uint8_t m0 = low_mac & 0xFF;
    uint8_t m1 = (low_mac >> 8) & 0xFF;
    uint8_t m2 = (low_mac >> 16) & 0xFF;
    uint8_t m3 = (low_mac >> 24) & 0xFF;
    uint8_t m4 = hi_mac & 0xFF;

    len = snprintf(buffer, bufsize,
                   "%d,%lu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%d,"
                   "%d,%d,%d,%u,%u,%02X,%02X,%02X,%02X,%02X:\r\n",
                   frame->Type_data, packet_id, serial_id, timestamp, s0, t0,
                   s1, t1, s2, t2, s3, t3, tag_id, motion, button, battery,
                   rssi, version, temp, m0, m1, m2, m3, m4);
  }
  else
  {
    len = snprintf(
        buffer, bufsize,
        "%d,%lu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%d,%d,%"
        "d,%d:\r\n",
        frame->Type_data, packet_id, serial_id, timestamp, s0, t0, s1, t1, s2,
        t2, s3, t3, tag_id, motion, button, battery, rssi);
  }

  return len;
}

// bool send_ds_twr_command(uint32_t target_id) {
//   if (target_id == 0) {
//     Serial.println(
//         "⚠️ Serial ID không hợp lệ. Vui lòng truyền target_id khác 0.");
//     return false;
//   }

//   Serial.printf("📡 Gửi lệnh đo khoảng cách DS-TWR đến Serial_ID: 0x%08lX
//   (%lu)\n", target_id, target_id);

//   uint8_t payload[4];
//   memcpy(payload, &target_id, 4);

//   uint8_t response[4]; // cmd, status, type, len
//   if (send_cmd_to_isp3080(CMD_DO_DS_TWR, payload, sizeof(payload), response,
//                           sizeof(response), 1000)) {
//     // Có thể xử lý phản hồi nếu cần
//     return true;
//   } else {
//     Serial.println("❌ Không gửi được lệnh DS-TWR");
//     return false;
//   }
// }

bool send_ds_twr_command(uint32_t target_id)
{
  if (target_id == 0)
  {
    dbg_dw_twr("%s",
        "⚠️ Serial ID không hợp lệ. Vui lòng truyền target_id khác 0.");
    return false;
  }

  dbg_dw_twr(
      "📡 Gửi lệnh đo khoảng cách DS-TWR đến Serial_ID: 0x%08lX (%lu)\n",
      target_id, target_id);

  uint8_t payload[4];
  memcpy(payload, &target_id, 4);
  uint8_t response[4];

  // Thử gửi tối đa 3 lần nếu ISP3080 chưa kịp phản hồi
  for (int retry = 0; retry < 5; retry++)
  {
    if (send_cmd_to_isp3080(CMD_DO_DS_TWR, payload, sizeof(payload), response,
                            sizeof(response), 500))
    {
      dbg_dw_twr("%s", "send DS_TWR command success");
      return true;
    }
    dbg_dw_twr("[U7] Retry %d: ISP chưa phản hồi, chờ 10ms...\n", retry + 1);
    delay(50);
  }

  dbg_dw_twr("%s", "❌ Không gửi được lệnh DS-TWR");
  return false;
}

// uint64_t parse_ts_40bit(const uint8_t ts[5])
// {
//     uint64_t val = 0;
//     for (int i = 4; i >= 0; i--)
//     {
//         val <<= 8;
//         val |= ts[i];
//     }
//     return val;
// }
uint64_t parse_ts_40bit(const uint8_t ts[5])
{
  uint64_t val = 0;
  for (int i = 0; i < 5; i++)
  {
    val |= ((uint64_t)ts[i]) << (8 * i);
  }
  return val;
}

void print_request_data(const request_data_t *rd)
{
  if (!rd)
    return;

  dbg_spi("%s", "========================================");
  dbg_spi("%s", "       U7 REQUEST DATA SUMMARY          ");
  dbg_spi("%s", "========================================");

  // 1. TIMER DATA
  dbg_spi("%s", "[TIMER DATA]");
  dbg_spi("  Motion: %u, Stand: %u\n", rd->timer.send_tag_motion,
                rd->timer.send_tag_stand);
  dbg_spi("  Sleep1: %u, Sleep2: %u, Sleep3: %u\n",
                rd->timer.send_tag_sleep1, rd->timer.send_tag_sleep2,
                rd->timer.send_tag_sleep3);
  dbg_spi("  Mode1: %u, Mode2: %u, Mode3: %u\n", rd->timer.sleep_mode1,
                rd->timer.sleep_mode2, rd->timer.sleep_mode3);
  dbg_spi("  Batt Default: %u, High: %u, Inc: %u, Dec: %u\n",
                rd->timer.batt_default, rd->timer.batt_high,
                rd->timer.batt_increase, rd->timer.batt_decrease);

  // 2. CONFIG DATA
  dbg_spi("%s", "\n[UWB CONFIG]");
  dbg_spi("  Chan: %u, Plen: %u, PAC: %u\n", rd->config.uwb_chan,
                rd->config.uwb_plen, rd->config.uwb_pac);
  dbg_spi("  TX/RX Code: %u/%u, SFD Type: %u\n", rd->config.uwb_txcode,
                rd->config.uwb_rxcode, rd->config.uwb_sfdtype);
  dbg_spi("  DataRate: %u, PHR Mode/Rate: %u/%u\n",
                rd->config.uwb_datarate, rd->config.uwb_phrmode,
                rd->config.uwb_phrrate);
  dbg_spi("  SFD TO: %u, STS Mode/Len: %u/%u, PDOA: %u\n",
                rd->config.uwb_sfdto, rd->config.uwb_stsmode,
                rd->config.uwb_stslen, rd->config.uwb_pdoa);

  // 3. STATE
  dbg_spi("\n[STATE]: %lu\n", rd->current_state);

  // 4. ID TAG
  dbg_spi("%s", "\n[ID & TAG]");
  dbg_spi("  ID: %02X:%02X:%02X:%02X:%02X\n", rd->id_tag.id[0],
                rd->id_tag.id[1], rd->id_tag.id[2], rd->id_tag.id[3],
                rd->id_tag.id[4]);

  dbg_spi("%s", "========================================\n");
}

// ============================================================================
// CÁC HÀM GIAO TIẾP OTA VỚI ISP3080
// ============================================================================

bool isp3080_ota_enter(uint32_t fw_size, uint32_t crc32_val)
{
  uint8_t payload[8];
  // fw_size (Little Endian)
  payload[0] = (uint8_t)(fw_size & 0xFF);
  payload[1] = (uint8_t)((fw_size >> 8) & 0xFF);
  payload[2] = (uint8_t)((fw_size >> 16) & 0xFF);
  payload[3] = (uint8_t)((fw_size >> 24) & 0xFF);
  // crc32 (Little Endian)
  payload[4] = (uint8_t)(crc32_val & 0xFF);
  payload[5] = (uint8_t)((crc32_val >> 8) & 0xFF);
  payload[6] = (uint8_t)((crc32_val >> 16) & 0xFF);
  payload[7] = (uint8_t)((crc32_val >> 24) & 0xFF);

  dbg_spi("[U7_OTA_SPI] === ENTER DEBUG START ===\n");
  dbg_spi(
      "[U7_OTA_SPI] payload: %02X %02X %02X %02X %02X %02X %02X %02X\n",
      payload[0], payload[1], payload[2], payload[3], payload[4], payload[5],
      payload[6], payload[7]);

  // Dọn session OTA cũ trước mỗi ENTER để tránh lần OTA kế tiếp bị reject
  // nếu ISP3080 còn state READY/RECEIVING/COMPLETE từ lần trước.
  dbg_spi("%s", "[U7_OTA_SPI] Pre-clean old OTA session with ABORT");
  bool abort_ok = false;
  for (int i = 0; i < 3; i++)
  {
    abort_ok = isp3080_ota_abort();
    dbg_spi("[U7_OTA_SPI] Pre-abort #%d -> %d\n", i, abort_ok);
    delay(100);
    if (abort_ok)
      break;
  }

  // Warm-up dummy để kéo response cũ / chờ SPIS rearm
  uint8_t test_resp[32] = {0};
  for (int i = 0; i < 5; i++)
  {
    memset(test_resp, 0, sizeof(test_resp));
    bool warm_ok = send_cmd_to_isp3080(0x00, nullptr, 0, test_resp,
                                       sizeof(test_resp), 100);
    dbg_spi("[U7_OTA_SPI] Warm-up #%d ok=%d resp=%02X %02X %02X %02X\n",
                  i, warm_ok, test_resp[0], test_resp[1], test_resp[2],
                  test_resp[3]);
    delay(50);
  }

  delay(300);

  is_sending_config = true;
  delay(15);

  // 1) Gửi lệnh CMD_OTA_ENTER = 0x20
  // QUAN TRỌNG: nRF52 SPIS có "dead zone" sau mỗi transaction (RELEASED state).
  // Nếu U7 gửi CMD đúng lúc SPIS chưa rearm → data bị mất (trả toàn 0xFF).
  // Giải pháp: retry gửi CMD cho đến khi SPIS trả non-0xFF (nghĩa là đã armed).
  uint8_t resp[32] = {0};
  bool cmd_captured = false;
  for (int attempt = 0; attempt < 20; attempt++)
  {
    bool cmd_ok =
        send_cmd_to_isp3080(0x20, payload, 8, resp, sizeof(resp), 500);
    if (cmd_ok)
    {
      cmd_captured = true;
      dbg_spi("[U7_OTA_SPI] ENTER attempt #%d: captured=%d, resp: %02X "
                    "%02X %02X %02X\n",
                    attempt, cmd_captured, resp[0], resp[1], resp[2], resp[3]);
      break;
    }
    delay(5); // Đợi SPIS rearm rồi thử lại
  }

  if (!cmd_captured)
  {
    dbg_spi("%s",
        "[U7_OTA_SPI] ENTER FAIL: SPIS không bao giờ armed (cmd bị mất)");
    is_sending_config = false;
    return false;
  }

  // 2) Kéo dummy để đọc ACK từ ISP3080
  for (int retry = 0; retry < 250; retry++)
  {
    delay(100);
    memset(resp, 0, sizeof(resp));
    bool poll_ok =
        send_cmd_to_isp3080(0x00, nullptr, 0, resp, sizeof(resp), 50);
    // In log mỗi 25 vòng (1 lần/0.5 giây) hoặc khi có dữ liệu
    if (poll_ok || (retry % 25 == 0))
    {
      dbg_spi("[U7_OTA_SPI] Poll #%d: ok=%d, resp: %02X %02X %02X %02X\n",
                    retry, poll_ok, resp[0], resp[1], resp[2], resp[3]);
    }
    if (poll_ok)
    {
      if (resp[0] == (0x20 | 0x80))
      {
        if (resp[1] == 0x00)
        {
          dbg_spi("%s", "[U7_OTA_SPI] ENTER OK!");
          // KEEP is_sending_config = true during OTA to avoid reset from Core 1
          return true;
        }
        dbg_spi("[U7_OTA_SPI] ENTER fail, mã lỗi: 0x%02X\n", resp[1]);
        is_sending_config = false;
        return false;
      }
    }
  }
  dbg_spi("%s", "[U7_OTA_SPI] ENTER Timeout (5s) - ISP3080 không phản hồi");
  is_sending_config = false;
  return false;
}

bool isp3080_ota_data(uint32_t offset, const uint8_t *data, uint32_t len)
{
  if (len > 480)
    return false; // MAX_OTA_CHUNK_SIZE
  uint8_t payload[512];
  // offset (Little Endian)
  payload[0] = (uint8_t)(offset & 0xFF);
  payload[1] = (uint8_t)((offset >> 8) & 0xFF);
  payload[2] = (uint8_t)((offset >> 16) & 0xFF);
  payload[3] = (uint8_t)((offset >> 24) & 0xFF);
  // len (Little Endian) - ota_isp.c expects uint16_t for len
  payload[4] = (uint8_t)(len & 0xFF);
  payload[5] = (uint8_t)((len >> 8) & 0xFF);

  memcpy(&payload[6], data, len);

  uint8_t resp[32] = {0};
  // is_sending_config đã được bật từ ENTER, giữ nguyên

  // Retry gửi CMD 0x21 cho đến khi SPIS armed
  bool cmd_captured = false;
  for (int attempt = 0; attempt < 1000; attempt++)
  {
    bool cmd_ok =
        send_cmd_to_isp3080(0x21, payload, 6 + len, resp, sizeof(resp), 500);
    if (cmd_ok)
    {
      cmd_captured = true;
      // Do NOT check resp[0] here or return early, because resp contains the
      // cached hardware response from the PREVIOUS transaction/poll. We MUST
      // proceed to the poll loop!
      break;
    }
    delay(3);
  }
  if (!cmd_captured)
  {
    dbg_spi("[U7_OTA_SPI] DATA FAIL: SPIS dead-zone (offset %lu)\n",
                  offset);
    return false;
  }

  // Poll ACK
  for (int retry = 0; retry < 1000; retry++)
  {
    delay(5);
    memset(resp, 0, sizeof(resp));
    bool poll_ok =
        send_cmd_to_isp3080(0x00, nullptr, 0, resp, sizeof(resp), 50);
    if (retry % 5 == 0 || poll_ok)
    {
      dbg_spi(
          "[U7_OTA_SPI] DATA Poll #%d, ok=%d, resp: %02X %02X %02X %02X\n",
          retry, poll_ok, resp[0], resp[1], resp[2], resp[3]);
    }

    if (poll_ok)
    {
      if (resp[0] == (0x21 | 0x80))
      {
        if (resp[1] == 0x00)
        {
          dbg_spi("%s", "[U7_OTA_SPI] DATA OK!");
          return true;
        }
        dbg_spi("[U7_OTA_SPI] DATA fail offset %lu, lỗi: 0x%02X\n",
                      offset, resp[1]);
        return false;
      }
    }
  }
  dbg_spi("[U7_OTA_SPI] DATA Timeout offset %lu\n", offset);
  return false;
}

bool isp3080_ota_end(void)
{
  uint8_t resp[32] = {0};
  is_sending_config = true; // Đề phòng
  delay(15);

  bool cmd_captured = false;
  for (int attempt = 0; attempt < 100; attempt++)
  {
    bool cmd_ok =
        send_cmd_to_isp3080(0x22, nullptr, 0, resp, sizeof(resp), 500);
    if (cmd_ok)
    {
      cmd_captured = true;
      break;
    }
    delay(5);
  }

  if (!cmd_captured)
  {
    dbg_spi("%s", "[U7_OTA_SPI] END FAIL: SPIS dead-zone");
    is_sending_config = false;
    return false;
  }

  // Quá trình End bao gồm chép flash & reboot trên ISP3080 nên đợi lên đến 30
  // giây
  for (int retry = 0; retry < 600; retry++)
  {
    delay(50);
    memset(resp, 0, sizeof(resp));
    if (send_cmd_to_isp3080(0x00, nullptr, 0, resp, sizeof(resp), 50))
    {
      if (resp[0] == 0x80 && resp[1] == 0x00)
      {
        // NRF52833 đã khởi động lại xong sau khi chép Flash! Thành công rực rỡ!
        dbg_spi("%s", "[U7_OTA_SPI] nRF52 da reboot va len duoc App. END OK!");
        is_sending_config = false;
        return true;
      }
      else if (resp[0] == 0xFE && resp[1] != 0x00 && resp[1] != 0xFE)
      {
        // Firmware cũ của nRF52 trả về FE <mã lỗi> thay vì A2 <mã lỗi>
        dbg_spi(
            "[U7_OTA_SPI] END fail (loi cu cua nRF52), ma loi: 0x%02X\n",
            resp[1]);
        is_sending_config = false;
        return false;
      }
      else if (resp[0] == (0x22 | 0x80))
      {
        if (resp[1] == 0x00)
        {
          dbg_spi("%s", "[U7_OTA_SPI] END OK!");
          is_sending_config = false;
          return true;
        }
        dbg_spi("[U7_OTA_SPI] END fail, ma loi: 0x%02X\n", resp[1]);
        is_sending_config = false;
        return false;
      }
    }
  }
  dbg_spi("%s", "[U7_OTA_SPI] END Timeout (kéo dummy 0x00 thất bại)");
  is_sending_config = false;
  return false;
}

bool isp3080_ota_abort(void)
{
  uint8_t resp[32] = {0};
  is_sending_config = true;
  delay(15);

  bool cmd_captured = false;
  for (int attempt = 0; attempt < 20; attempt++)
  {
    bool cmd_ok =
        send_cmd_to_isp3080(0x23, nullptr, 0, resp, sizeof(resp), 500);
    if (cmd_ok)
    {
      cmd_captured = true;
      break;
    }
    delay(5);
  }

  if (!cmd_captured)
  {
    dbg_spi("%s", "[U7_OTA_SPI] ABORT FAIL: SPIS dead-zone");
    is_sending_config = false;
    return false;
  }

  for (int retry = 0; retry < 20; retry++)
  {
    delay(5);
    memset(resp, 0, sizeof(resp));
    if (send_cmd_to_isp3080(0x00, nullptr, 0, resp, sizeof(resp), 50))
    {
      if (resp[0] == (0x23 | 0x80))
      {
        if (resp[1] == 0x00)
        {
          is_sending_config = false;
          return true;
        }
        dbg_spi("[U7_OTA_SPI] ABORT fail, lỗi: 0x%02X\n", resp[1]);
        is_sending_config = false;
        return false;
      }
    }
  }
  dbg_spi("%s", "[U7_OTA_SPI] ABORT Timeout (kéo dummy 0x00 thất bại)");
  is_sending_config = false;
  return false;
}

// ============================================================
// Đọc ISP3080 FW/HW version qua SPI (retry 3 lần)
// ISP trả về chuỗi "FW:x.xx-HW:x.xx" ở resp[2..]
// ============================================================
bool read_isp_version(char *fw_ver, size_t fw_len, char *hw_ver,
                      size_t hw_len)
{
  const int retry_limit = 10;
  uint8_t resp[34] = {0};

  for (int attempt = 1; attempt <= retry_limit; attempt++)
  {
    memset(resp, 0, sizeof(resp));

    if (!send_cmd_to_isp3080(CMD_GET_ISP_VERSION, nullptr, 0, resp,
                             sizeof(resp), 50))
    {
      dbg_spi("[ISP_VER] SPI timeout (lần %d/%d)\n", attempt,
                    retry_limit);
      delay(100);
      continue;
    }

    if (resp[0] != (CMD_GET_ISP_VERSION | 0x80) || resp[1] != 0x00)
    {
      dbg_spi(
          "[ISP_VER] Lỗi phản hồi cmd=0x%02X status=0x%02X (lần %d)\n", resp[0],
          resp[1], attempt);
      delay(50);
      continue;
    }

    char *ver_str = (char *)&resp[2];
    dbg_spi("[ISP_VER] ISP Version raw: %s\n", ver_str);

    char *fw_ptr = strstr(ver_str, "FW:");
    char *hw_ptr = strstr(ver_str, "HW:");

    if (fw_ptr)
    {
      fw_ptr += 3;
      char *dash = strchr(fw_ptr, '-');
      size_t len = dash ? (size_t)(dash - fw_ptr) : strlen(fw_ptr);
      if (len >= fw_len)
        len = fw_len - 1;
      strncpy(fw_ver, fw_ptr, len);
      fw_ver[len] = '\0';
    }

    if (hw_ptr)
    {
      hw_ptr += 3;
      size_t len = strlen(hw_ptr);
      if (len >= hw_len)
        len = hw_len - 1;
      strncpy(hw_ver, hw_ptr, len);
      hw_ver[len] = '\0';
    }

    return true;
  }

  dbg_spi("%s", "[ISP_VER] Đọc ISP version thất bại");
  return false;
}
