#include "main.h"
#include "app_timer.h"
#include "nrf_nvic.h"
// #include "ducthangtest.h"
#include "DW3000.h"
#include "broadcast_twr.h"
#include "ducthang_flash.h"
#include "ducthangble.h"
#include "nrf_sdh_soc.h"
#include "nrf_soc.h"
#include "ota_isp.h"
#include "request.h"
#include "ss_twr_ducthang.h"

// ==== Bộ đệm giao tiếp SPI ====
uint8_t m_tx_buf[BUF_LEN];
uint8_t m_rx_buf[BUF_LEN];
uint8_t m_rx_copy[RX_BUF_SIZE];

// ==== TDoA Server FIFO ====
static tdoa_server_queue_t tdoa_server_queue = {0};
static volatile bool tdoa_spi_frame_pending = false;
volatile uint32_t tdoa_uwb_received = 0;
volatile uint32_t tdoa_queue_pushed = 0;
volatile uint32_t tdoa_queue_dropped = 0;
volatile uint32_t tdoa_spi_popped = 0;

void tdoa_update_irq(void)
{
  if (tdoa_spi_frame_pending)
  {
    // U7 hiện bắt IRQ bằng RISING, nên phát xung ngắn mỗi khi đã nạp sẵn
    // một frame vào SPIS. Không giữ HIGH liên tục ở bản tương thích này.
    TAG_IRQ_SET();
    nrf_delay_us(10);
    TAG_IRQ_CLR();
  }
  else
  {
    TAG_IRQ_CLR();
  }
}

bool tdoa_queue_push(const server_dataframe_t *frame)
{
  if (tdoa_server_queue.count >= TDOA_SERVER_QUEUE_SIZE)
  {
    tdoa_queue_dropped++;
    return false;
  }

  tdoa_server_queue.frames[tdoa_server_queue.write_index] = *frame;
  tdoa_server_queue.write_index =
      (tdoa_server_queue.write_index + 1) % TDOA_SERVER_QUEUE_SIZE;
  tdoa_server_queue.count++;
  tdoa_queue_pushed++;
  return true;
}

bool tdoa_prepare_spi_frame(void)
{
  if (tdoa_spi_frame_pending || tdoa_server_queue.count == 0)
  {
    return false;
  }

  server_dataframe_t *front =
      &tdoa_server_queue.frames[tdoa_server_queue.read_index];

  m_tx_buf[0] = CMD_GET_FRAME_SERVER | 0x80;
  m_tx_buf[1] = OK_STATUS;
  m_tx_buf[2] = FRAME_TYPE_SERVER;
  m_tx_buf[3] = sizeof(server_dataframe_t);
  memcpy(&m_tx_buf[4], front, sizeof(server_dataframe_t));
  memset(&m_tx_buf[4 + sizeof(server_dataframe_t)], 0xFF,
         BUF_LEN - 4 - sizeof(server_dataframe_t));

  nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN);
  tdoa_spi_frame_pending = true;
  tdoa_update_irq();
  return true;
}

static void tdoa_complete_spi_read(void)
{
  if (!tdoa_spi_frame_pending)
  {
    return;
  }

  if (tdoa_server_queue.count > 0)
  {
    tdoa_server_queue.read_index =
        (tdoa_server_queue.read_index + 1) % TDOA_SERVER_QUEUE_SIZE;
    tdoa_server_queue.count--;
    tdoa_spi_popped++;
  }

  tdoa_spi_frame_pending = false;
}

// ==== BSS-TWR Result Queue ====
twr_queue_entry_t twr_result_queue[TWR_RESULT_QUEUE_SIZE];
uint8_t twr_q_head = 0;
uint8_t twr_q_tail = 0;
uint8_t twr_q_count = 0;

void twr_queue_push(uint8_t *tag_id, uint32_t dist_mm)
{
  if (twr_q_count >= TWR_RESULT_QUEUE_SIZE)
  {
    // Queue đầy: đẩy head đi (overwrite oldest)
    twr_q_head = (twr_q_head + 1) % TWR_RESULT_QUEUE_SIZE;
    twr_q_count--;
  }
  memcpy(twr_result_queue[twr_q_tail].tag_id, tag_id, 5);
  twr_result_queue[twr_q_tail].distance_mm = dist_mm;
  twr_q_tail = (twr_q_tail + 1) % TWR_RESULT_QUEUE_SIZE;
  twr_q_count++;
}

bool twr_queue_pop(twr_queue_entry_t *out)
{
  if (twr_q_count == 0)
    return false;
  *out = twr_result_queue[twr_q_head];
  twr_q_head = (twr_q_head + 1) % TWR_RESULT_QUEUE_SIZE;
  twr_q_count--;
  return true;
}

// ==== Trạng thái hệ thống ====
bool enable_polling_rx = false;
uint32_t MY_DEVICE_ID = 0;
dw_mode_run mode_run = mode_tdoa;     // Chế độ mặc định là TDOA
uint32_t ds_twr_target_base_id = 0;   // Target riêng cho DS-TWR base-base
uint32_t ss_twr_target_tag_id = 0;    // Target số cho log/legacy SS-TWR tag unicast
uint8_t ss_twr_target_tag_raw[5];     // Target raw 5 byte cho SS-TWR tag HEX/MAC
uint8_t my_base_id_raw[5];            // ID base (5 byte raw)
uint8_t current_tag_id_raw[5];        // ID tag đang đo (5 byte raw)
bool flag_send_start_twr = false;     // Flag cho DS-TWR cũ
bool flag_start_ducthang_twr = false; // Flag RIÊNG cho DucThang SS-TWR
bool flag_init_bcast_twr = false;     // Cờ khởi tạo Queue cho Broadcast TWR
static bool unicast_twr_timer_active = false;
static uint32_t unicast_twr_start_ticks = 0;
#define UNICAST_TWR_TIMEOUT_TICKS APP_TIMER_TICKS(2000)
#define UNICAST_TWR_RESULT_COUNT 20
static uint8_t unicast_twr_results_sent = 0;
static volatile bool unicast_twr_result_waiting = false;
static volatile bool unicast_twr_result_read_done = false;
static bool unicast_twr_release_waiting = false;
volatile bool flag_request_data_ready = false;
volatile bool flag_ack_status_ready = false;

static void reset_unicast_twr_session(void)
{
  unicast_twr_timer_active = false;
  unicast_twr_start_ticks = 0;
  unicast_twr_results_sent = 0;
  unicast_twr_result_waiting = false;
  unicast_twr_result_read_done = false;
  unicast_twr_release_waiting = false;
}

static void restore_dw3000_to_tdoa(void)
{
  flag_start_ducthang_twr = false;
  reset_unicast_twr_session();
  ss_twr_ducthang_cleanup();
  mode_run = mode_tdoa;
  dwt_forcetrxoff();
  begin_dw3000();
  dwt_writesysstatuslo(SYS_STATUS_ALL_RX_ERR);
  int rx_ret = dwt_rxenable(DWT_START_RX_IMMEDIATE);
  enable_polling_rx = (rx_ret == DWT_SUCCESS);
}

// ==== Cấu hình Beacon (Toàn cục) ====
// g_beacon_cfg, g_pending_beacon_cfg, flag_save_config_pending,
// last_ducthang_ms_cnt are now managed by ducthang_flash.h

#define PIN_RESET_BY_GPIO NRF_GPIO_PIN_MAP(0, 25)

// ==== Hàm khởi tạo chân reset mềm ====
static void reset_pin_init(void)
{
  nrf_gpio_cfg_input(PIN_RESET_BY_GPIO,
                     NRF_GPIO_PIN_PULLUP); // input có pull-up
}

const uint32_t my_device_id = 125201;

// ==== In buffer dưới dạng hex ====
static void dump_bytes(const uint8_t *buf, size_t len)
{
  // for (size_t i = 0; i < len; i++)
  //   printf("%02X ", buf[i]);
  // printf("\r\n");
}

// ==== Xử lý sự kiện SPIS ====
static void spis_evt_handler(nrfx_spis_evt_t const *evt, void *ctx)
{
  if (evt->evt_type == NRFX_SPIS_XFER_DONE)
  {
    spis_rx_len = evt->rx_amount;
    spis_xfer_done = true;
    // Copy dữ liệu nhận được ra m_rx_copy để xử lý
    memcpy(m_rx_copy, m_rx_buf, spis_rx_len);

    // Nếu transaction vừa xong đang trả frame TDoA, chỉ lúc này mới pop FIFO.
    // Nhờ vậy frame không bị ghi đè/mất trước khi ESP32 đọc xong qua SPI.
    tdoa_complete_spi_read();

    // Chỉ đánh dấu trong ISR. Vòng main sẽ tăng bộ đếm và chuyển lượt đo.
    // Khi cờ waiting bật, m_tx_buf đang giữ đúng frame kết quả Unicast hiện tại;
    // vì vậy transaction vừa hoàn tất đồng nghĩa U7 đã đọc frame này.
    if (unicast_twr_result_waiting)
    {
      unicast_twr_result_read_done = true;
    }
  }
}

// ==== Khởi tạo chân báo IRQ cho ESP32 ====
static void irq_esp_init(void)
{
  nrf_gpio_cfg_output(PIN_TAG_IRQ);
  TAG_IRQ_CLR(); // đặt LOW ban đầu
}

// ==== Khởi tạo SPIS1 ====
static void spis_init(void)
{
  memset(m_tx_buf, 0xFF, sizeof(m_tx_buf));
  memset(m_rx_buf, 0x00, sizeof(m_rx_buf));

  nrfx_spis_config_t cfg = NRFX_SPIS_DEFAULT_CONFIG;
  cfg.sck_pin = SPIS_SCK_PIN;
  cfg.mosi_pin = SPIS_MOSI_PIN;
  cfg.miso_pin = SPIS_MISO_PIN;
  cfg.csn_pin = SPIS_CSN_PIN;
  cfg.mode = NRF_SPIS_MODE_0;
  cfg.bit_order = NRF_SPIS_BIT_ORDER_MSB_FIRST;
  cfg.orc = 0xFF;

  irq_esp_init(); // ⚡️ Chân báo ESP32

  APP_ERROR_CHECK(nrfx_spis_init(&spis, &cfg, spis_evt_handler, NULL));
  APP_ERROR_CHECK(
      nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN));
}

// ==== Kiểm tra nút reset mềm ====
void check_reset_pin(void)
{
  if (nrf_gpio_pin_read(PIN_RESET_BY_GPIO) == 0)
  {
    // printf("[SYS] RESET nRF52833 - P0.25\n");
    NVIC_SystemReset(); // reset phần mềm
  }
}

// ==== Cập nhật ID cho DucThang SS-TWR ====
static void update_ducthang_ids(void)
{
  uint32_t temp_id = MY_DEVICE_ID;
  for (int i = 4; i >= 0; i--)
  {
    uint8_t two_digits = temp_id % 100;
    temp_id /= 100;
    my_base_id_raw[i] = ((two_digits / 10) << 4) | (two_digits % 10);
  }

  // SS-TWR chỉ được lấy Tag ID từ cấu hình tag hoặc bản raw đã cache.
  // Không fallback sang DS-TWR BaseID vì sẽ làm base-tag đo nhầm base-base cũ.
  bool tag_id_empty = true;
  bool cached_tag_empty = true;
  for (int i = 0; i < 5; i++)
  {
    if (g_beacon_cfg.val_id_last[i] != 0)
    {
      tag_id_empty = false;
    }
    if (ss_twr_target_tag_raw[i] != 0)
    {
      cached_tag_empty = false;
    }
  }

  if (!tag_id_empty)
  {
    memcpy(current_tag_id_raw, g_beacon_cfg.val_id_last, 5);
    memcpy(ss_twr_target_tag_raw, g_beacon_cfg.val_id_last, 5);
  }
  else if (!cached_tag_empty)
  {
    memcpy(current_tag_id_raw, ss_twr_target_tag_raw, 5);
  }
  // Nếu không có ID mới và cũng chưa có cache, giữ nguyên current_tag_id_raw
  // để tránh xóa target đang đo khi g_beacon_cfg bị clear sau khi phát BLE.
}

// ==== HÀM MAIN ====
int main(void)
{
  printf("=== ISP3080 OTA 14/4/2026 ===\r\n");

  bsp_board_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS);
  APP_ERROR_CHECK(app_timer_init());
  ducthang_ble_init();
  load_dwt_config(); // Tải config từ flash
  request_init();    // Khởi tạo request handlers
  ota_isp_init();    // Khởi tạo module OTA qua SPI

  gpio_init();
  reset_pin_init();
  nrf52840_dk_spi_init(); // SPI cho DW3000
  spis_init();            // SPI Slave cho ESP32

  dw_irq_init(); // IRQ chân DIO

  // printf("Reset DW3000...\r\n");
  reset_DWIC();
  // Sleep(10);
  //  ==== Khởi tạo DW3000 ====
  //  printf("DW3000 SPI fastrate...\r\n");
  port_set_dw_ic_spi_fastrate();
  // ==== Chuẩn bị buffer SPIS ban đầu ====

  // ==== Vòng lặp chính ====
  while (1)
  {
    /*─── OTA Mode Guard: khi đang OTA, bỏ qua toàn bộ TDOA/BLE ───*/
    if (ota_isp_is_active())
    {
      /* Nếu OTA hoàn tất (CRC OK), copy firmware và reboot */
      if (ota_isp_get_state() == OTA_STATE_COMPLETE)
      {
        ota_isp_apply_and_reboot(); /* Không return nếu thành công */
      }
      /* Trong OTA mode, chỉ xử lý SPI commands, nhảy thẳng xuống SPI handler */
      goto ota_spi_only;
    }

    if (flag_save_config_pending)
    {
      flag_save_config_pending = false;

      // Chỉ áp dụng cấu hình và reset DW3000 nếu có cờ apply_config_to_base
      if (g_beacon_cfg.apply_config_to_base == 1)
      {
        apply_and_save_dwt_config();

        // Reset cờ sau khi đã áp dụng xong để tránh lưu vào log SPI/Flash hoặc
        // lưu cache
        g_beacon_cfg.apply_config_to_base = 0;

        // Apply the configurations to DWM
        dwt_forcetrxoff();
        // Phải khởi tạo lại toàn bộ luồng bằng begin_dw3000 áp dụng config mới
        // nhất.
        begin_dw3000();

        if (enable_polling_rx)
        {
          dwt_rxenable(DWT_START_RX_IMMEDIATE);
        }
      }
    }

    // Khởi tạo Broadcast TWR Queue khi nhận được lệnh (tách riêng khỏi cấu hình
    // DWT)
    if (flag_init_bcast_twr)
    {
      flag_init_bcast_twr = false;
      if (g_beacon_cfg.val_id_mode == 5 && g_beacon_cfg.enable_bcast_twr)
      {
        printf("[nRF] Initializing Broadcast TWR Queue...\r\n");
        bcast_twr_init();
        // Khởi động BLE advertising + scan để Tag nhìn thấy Base
        start_config_advertising();
      }
      else
      {
        printf("[nRF] flag_init_bcast_twr SET but condition not met: mode=%d, "
               "bcast_en=%d\r\n",
               g_beacon_cfg.val_id_mode, g_beacon_cfg.enable_bcast_twr);
      }
    }

    // TDOA chỉ được dùng radio khi SS-TWR broadcast/unicast chưa sở hữu DW3000.
    // Unicast chờ BLE ACK chưa bật flag nên TDOA vẫn tiếp tục nhận bình thường.
    if (enable_polling_rx && !g_beacon_cfg.enable_bcast_twr &&
        !flag_start_ducthang_twr)
    {
      poll_rx_once(); // nhận gói tin từ DW3000
    }

    // ==== Tự động kích hoạt DucThang SS-TWR liên tục nếu Mode=5 ====
    if (g_beacon_cfg.val_id_mode == 5)
    {
      if (g_beacon_cfg.enable_bcast_twr)
      {
        enable_polling_rx = false; // Broadcast TWR giữ logic cũ: tắt TDOA khi vào BSS-TWR
        bcast_twr_enable();
        bcast_twr_process(); // Scan BLE
        if (bcast_twr_next_tag(current_tag_id_raw))
        {
          flag_start_ducthang_twr = true;
        }
        else
        {
          flag_start_ducthang_twr = false;
        }
      }
      else
      {
        update_ducthang_ids();
        if (flag_start_ducthang_twr)
        {
          // Unicast: flag này chỉ được bật sau khi BLE ACK đủ fragment.
          enable_polling_rx = false;
        }
        else
        {
          // Đang phát BLE config/chờ ACK -> không chặn TDOA.
          enable_polling_rx = true;
        }
      }
    }
    else
    {
      // Khi thoát khỏi Mode 5 (sang Mode khác), phải tắt ngay cờ đo TWR
      if (flag_start_ducthang_twr || g_beacon_cfg.enable_bcast_twr)
      {
        bcast_twr_disable();
        ducthang_ble_stop_all_timers(); // <-- FIX CRASH: Phải tắt toàn bộ BLE
                                        // Timer (30s) trước khi thoát để
                                        // SoftDevice không gọi các hàm quét gây
                                        // sập

        flag_start_ducthang_twr = false;
        g_beacon_cfg.enable_bcast_twr =
            0; // Tắt luôn cờ Bcast để Timer không nhảy lại

        // Cleanup SS-TWR state (reset config_applied flag)
        ss_twr_ducthang_cleanup();
        // Khôi phục DW3000 về cấu hình mặc định bằng begin_dw3000()
        mode_run = mode_tdoa;
        dwt_forcetrxoff();
        begin_dw3000();
        dwt_writesysstatuslo(SYS_STATUS_ALL_RX_ERR);
        int rx_ret = dwt_rxenable(DWT_START_RX_IMMEDIATE);
        enable_polling_rx = (rx_ret == DWT_SUCCESS);
        // printf("[nRF] Exiting SS-TWR -> Restored default DW3000 config "
        //        "(polling_rx=%d)\n",
        //        enable_polling_rx);
      }

      // Log periodically if mode is not 5
      // static uint32_t mode_cnt = 0;
      // if (++mode_cnt > 1000000) {
      //   mode_cnt = 0;
      //   printf("[nRF] Heartbeat - Mode: %u, polling_rx: %d\r\n",
      //          g_beacon_cfg.val_id_mode, enable_polling_rx);
      // }
    }

    // ==== Logic RIÊNG của DucThang SS-TWR (Mode 5) ====
    if (flag_start_ducthang_twr)
    {
      const bool is_broadcast_twr = (g_beacon_cfg.enable_bcast_twr != 0);

      // Timer chỉ dành cho unicast và bắt đầu khi unicast thực sự sở hữu radio.
      if (is_broadcast_twr)
      {
        unicast_twr_timer_active = false;
      }
      else if (!unicast_twr_timer_active)
      {
        unicast_twr_start_ticks = app_timer_cnt_get();
        unicast_twr_timer_active = true;
      }

      bool any_success = false;
      float best_dist = -1.0f;

      // Broadcast luôn giữ nguyên 5 lần thử. Unicast chỉ được đo khi frame kết quả
      // trước đó đã được U7 đọc xong, tránh vừa giữ SPI buffer vừa chiếm radio.
      if (is_broadcast_twr || !unicast_twr_result_waiting)
      {
        for (int t = 0; t < 5; t++)
        {
          bool success = ss_twr_ducthang_handle();
          if (success)
          {
            any_success = true;
            float cur_dist = ss_twr_ducthang_get_distance();
            if (best_dist < 0.0f || cur_dist < best_dist)
            {
              best_dist = cur_dist;
            }
            break;
          }
        }
      }

      if (is_broadcast_twr)
      {
        // Không thay đổi hành vi broadcast: success/fail đều kết thúc lượt Tag
        // hiện tại để scheduler round-robin tiếp tục Tag kế tiếp.
        if (any_success)
        {
          uint32_t distance_mm = (uint32_t)(best_dist * 100.0f);
          bcast_twr_finish_current(true);
          twr_queue_push(current_tag_id_raw, distance_mm);
        }
        else
        {
          bcast_twr_finish_current(false);
        }
      }
      else
      {
        // Giữ nguyên frame cho tới khi U7 đọc xong. Không đo lượt kế tiếp và
        // không cho producer SPI khác ghi đè m_tx_buf trong thời gian này.
        if (unicast_twr_result_read_done)
        {
          unicast_twr_result_read_done = false;
          unicast_twr_results_sent++;

          if (unicast_twr_results_sent >= UNICAST_TWR_RESULT_COUNT)
          {
            // Frame thứ 20 đã được U7 đọc xong: kết thúc phiên và trả radio về TDOA.
            restore_dw3000_to_tdoa();
          }
          else
          {
            // Giữ khóa buffer hết vòng hiện tại để SPI command handler xử lý
            // transaction vừa hoàn tất. Vòng sau mới mở khóa và đo lượt mới.
            unicast_twr_timer_active = false;
            unicast_twr_release_waiting = true;
          }
        }
        else if (unicast_twr_release_waiting)
        {
          unicast_twr_release_waiting = false;
          unicast_twr_result_waiting = false;
        }

        if (flag_start_ducthang_twr && !unicast_twr_result_waiting)
        {
          if (!unicast_twr_timer_active)
          {
            unicast_twr_start_ticks = app_timer_cnt_get();
            unicast_twr_timer_active = true;
          }

          bool unicast_timed_out = false;
          if (!any_success)
          {
            uint32_t elapsed_ticks = app_timer_cnt_diff_compute(
                app_timer_cnt_get(), unicast_twr_start_ticks);
            unicast_timed_out = (elapsed_ticks >= UNICAST_TWR_TIMEOUT_TICKS);
          }

          if (any_success || unicast_timed_out)
          {
            // Mỗi lượt tạo đúng một frame: khoảng cách mm hoặc -1 sentinel.
            dw_dataframe_t dist_frame = {0};
            dist_frame.Cmd = Cmd_Distance;
            dist_frame.Src = ss_twr_target_tag_id;
            dist_frame.Des = MY_DEVICE_ID;
            dist_frame.Data.DIST.Dis = any_success
                                           ? (uint32_t)(best_dist * 1000.0f)
                                           : UINT32_MAX;

            memcpy(&m_tx_buf[4], &dist_frame, sizeof(dw_dataframe_t));
            m_tx_buf[0] = CMD_GET_FRAME_DW | 0x80;
            m_tx_buf[1] = OK_STATUS;
            m_tx_buf[2] = FRAME_TYPE_DW;
            m_tx_buf[3] = sizeof(dw_dataframe_t);
            memset(&m_tx_buf[4 + sizeof(dw_dataframe_t)], 0xFF,
                   BUF_LEN - 4 - sizeof(dw_dataframe_t));

            unicast_twr_result_waiting = true;
            nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN);
            TAG_IRQ_SET();
            nrf_delay_us(10);
            TAG_IRQ_CLR();
          }
          // Chưa thành công và chưa đủ 2 giây: giữ lượt để batch sau retry.
        }
      }
    }
    else if (!g_beacon_cfg.enable_bcast_twr)
    {
      // Cờ có thể bị hạ bởi một cấu hình mới trước khi phép đo bắt đầu.
      reset_unicast_twr_session();
    }

    if (flag_send_start_twr)
    {
      send_start_twr(ds_twr_target_base_id);
      flag_send_start_twr = false;
    }

    // check_reset_pin(); // kiểm tra nút reset mềm

    // ==== Logic Gửi dữ liệu Request Data sang ESP32 ====
    if (flag_request_data_ready && !unicast_twr_result_waiting)
    {
      flag_request_data_ready = false;

      m_tx_buf[0] = CMD_GET_REQUEST_DATA | 0x80;
      m_tx_buf[1] = OK_STATUS;
      m_tx_buf[2] = FRAME_TYPE_REQUEST;
      m_tx_buf[3] = sizeof(request_data_t);
      memcpy(&m_tx_buf[4], &g_request_data, sizeof(request_data_t));
      memset(&m_tx_buf[4 + sizeof(request_data_t)], 0xFF,
             BUF_LEN - 4 - sizeof(request_data_t));

      nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN);
      TAG_IRQ_SET();
      TAG_IRQ_CLR();
    }

    // ==== Logic Gửi trạng thái ACK (Fragment Status) sang ESP32 ====
    if (flag_ack_status_ready && !unicast_twr_result_waiting)
    {
      flag_ack_status_ready = false;

      m_tx_buf[0] = CMD_GET_FRAGMENT_STATUS | 0x80;
      m_tx_buf[1] = OK_STATUS;
      m_tx_buf[2] = FRAME_TYPE_ACK_STATUS;
      m_tx_buf[3] = sizeof(uint64_t);
      memcpy(&m_tx_buf[4], &fragment_status, sizeof(uint64_t));
      memset(&m_tx_buf[4 + sizeof(uint64_t)], 0xFF,
             BUF_LEN - 4 - sizeof(uint64_t));

      nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN);
      TAG_IRQ_SET();
      nrf_delay_us(10);
      TAG_IRQ_CLR();
    }

    // ==== Logic Gửi trạng thái BCAST TWR Result (giống ACK) ====
    // Biến static báo hiệu rằng m_tx_buf đang chứa BCAST TWR Result và đang chờ
    // ESP32 đọc
    static bool is_waiting_esp32_read = false;

    // Lấy khỏi queue nếu hàng đợi có dữ liệu và buffer chưa bị chiếm
    if (!is_waiting_esp32_read && !unicast_twr_result_waiting &&
        twr_q_count > 0 && !flag_request_data_ready &&
        !flag_ack_status_ready)
    {
      twr_queue_entry_t entry;
      if (twr_queue_pop(&entry))
      {
        dw_twr_result_t res;
        memcpy(res.tag_id, entry.tag_id, 5);
        res.distance_mm = entry.distance_mm;

        m_tx_buf[0] = CMD_GET_BCAST_TWR_RESULT | 0x80;
        m_tx_buf[1] = OK_STATUS;
        m_tx_buf[2] = FRAME_TYPE_BCAST_TWR_RESULT;
        m_tx_buf[3] = sizeof(dw_twr_result_t);
        memcpy(&m_tx_buf[4], &res, sizeof(dw_twr_result_t));
        memset(&m_tx_buf[4 + sizeof(dw_twr_result_t)], 0xFF,
               BUF_LEN - 4 - sizeof(dw_twr_result_t));

        nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN);

        is_waiting_esp32_read = true; // Khóa buffer lại cho đến khi ESP32 đọc

        // Đánh IRQ để ESP32 biết kéo xuống PULL
        TAG_IRQ_SET();
        nrf_delay_us(10);
        TAG_IRQ_CLR();
      }
    }

    // ==== Logic Gửi dữ liệu TDoA sang ESP32 ====
    // Nạp frame đầu FIFO khi buffer SPI rảnh. IRQ sẽ giữ HIGH cho tới khi U7 đọc
    // hết các frame đang chờ, tránh mất xung khi nhiều Tag phát gần nhau.
    if (!is_waiting_esp32_read && !unicast_twr_result_waiting &&
        !flag_request_data_ready && !flag_ack_status_ready)
    {
      tdoa_prepare_spi_frame();
    }

  // ==== Nhãn nhảy: trong OTA mode, chỉ xử lý SPI ====
  ota_spi_only:

    // ==== Khi nhận được command SPI từ ESP32 ====
    if (spis_xfer_done)
    {
      spis_xfer_done = false;
      is_waiting_esp32_read =
          false; // Đã truyền vòng quay xong, mở khóa buffer lấy Tag tiếp theo

      if (spis_rx_len < 1)
      {
        // printf("[WARN] SPI: empty cmd\n");
        goto next_spi_ready;
      }

      uint8_t cmd = m_rx_copy[0];
      // printf("[RX] CMD = 0x%02X\r\n", cmd);
      //   dump_bytes(m_rx_copy, spis_rx_len);

      // ==== Xử lý từng command ====
      switch (cmd)
      {

      case 0x00:
      {
        // Lệnh PING/POLL: Trả về trạng thái OK để ESP32 biết nRF còn sống
        if (ota_isp_get_state() == 0)
        {                     // OTA_STATE_IDLE
          m_tx_buf[0] = 0x80; // Phản hồi 0x00 | 0x80
          m_tx_buf[1] = OK_STATUS;
          // Giữ nguyên các byte còn lại của buffer
        }
        break;
      }

      case CMD_BEGIN_DW3000:
      {
        // printf("[CMD] BEGIN_DW3000\n");
        uint8_t ret = begin_dw3000();
        m_tx_buf[0] = CMD_BEGIN_DW3000 | 0x80;
        m_tx_buf[1] = ret;
        memset(&m_tx_buf[2], 0xFF, BUF_LEN - 2);

        // has_tag_frame_ready = false;
        break;
      }

      case CMD_GET_ISP_VERSION:
      {
        // Trả về chuỗi "FW:x.xx-HW:x.xx" cho U7
        char ver_str[32];
        snprintf(ver_str, sizeof(ver_str), "FW:%s-HW:%s", ISP_FW_VERSION,
                 ISP_HW_VERSION);
        m_tx_buf[0] = CMD_GET_ISP_VERSION | 0x80;
        m_tx_buf[1] = OK_STATUS;
        memcpy(&m_tx_buf[2], ver_str, strlen(ver_str) + 1);
        memset(&m_tx_buf[2 + strlen(ver_str) + 1], 0xFF,
               BUF_LEN - 2 - strlen(ver_str) - 1);
        break;
      }

      case CMD_ENABLE_RX:
      {
        // printf("[CMD] ENABLE_RX\n");
        mode_run = mode_tdoa;
        dwt_writesysstatuslo(SYS_STATUS_ALL_RX_ERR);
        int ret = dwt_rxenable(DWT_START_RX_IMMEDIATE);
        enable_polling_rx = (ret == DWT_SUCCESS);

        m_tx_buf[0] = CMD_ENABLE_RX | 0x80;
        m_tx_buf[1] = (ret == DWT_SUCCESS) ? 0x00 : 0x01;
        memset(&m_tx_buf[2], 0xFF, BUF_LEN - 2);

        // has_tag_frame_ready = false;

        // printf("enable_polling_rx: %d\n", enable_polling_rx);
        // printf("DEV_ID: 0x%08lX\n", dwt_readdevid());
        break;
      }

      case CMD_SET_DEVICE_ID:
      {
        uint32_t new_id = 0;
        new_id |= ((uint32_t)m_rx_copy[1] << 0);
        new_id |= ((uint32_t)m_rx_copy[2] << 8);
        new_id |= ((uint32_t)m_rx_copy[3] << 16);
        new_id |= ((uint32_t)m_rx_copy[4] << 24);

        MY_DEVICE_ID = new_id;

        MY_DEVICE_ID = new_id;

        // Phan hoi thanh cong
        m_tx_buf[0] = CMD_SET_DEVICE_ID | 0x80;
        m_tx_buf[1] = 0x00;
        memset(&m_tx_buf[2], 0xFF, BUF_LEN - 2);
        break;
      }

      case CMD_SET_BEACON_STR:
      {
        if (spis_rx_len >= sizeof(beacon_cfg_t) + 1)
        {
          bool was_bcast_twr_enabled =
              (g_beacon_cfg.enable_bcast_twr != 0);

          // Nếu đang ở BSS-TWR mode, phải clean exit scheduler trước.
          if (was_bcast_twr_enabled)
          {
            printf("[nRF] Exiting BSS-TWR mode (new config received)\n");
            bcast_twr_disable();
            flag_start_ducthang_twr = false;
            // Tắt TẤT CẢ timer BLE (bao gồm timer global 30s)
            ducthang_ble_stop_all_timers();
          }

          memcpy(&g_beacon_cfg, &m_rx_copy[1], sizeof(beacon_cfg_t));

          // Cạnh bật -> tắt phải luôn khôi phục timing/RX mặc định của TDOA.
          if (was_bcast_twr_enabled && !g_beacon_cfg.enable_bcast_twr)
          {
            restore_dw3000_to_tdoa();
          }

          // Nếu config mới bật BSS-TWR → init queue
          if (g_beacon_cfg.enable_bcast_twr && g_beacon_cfg.val_id_mode == 5)
          {
            printf("[nRF] Initializing Broadcast TWR Queue...\n");
            bcast_twr_init();
          }

          // Bắt đầu phát cấu hình (bên trong hàm này sẽ gọi
          // ducthang_ble_update_from_cfg)
          start_config_advertising();

          printf("[nRF] Da nhan CMD_SET_BEACON_STR (Size: %d, bcast=%d, "
                 "mode=%d)\n",
                 sizeof(beacon_cfg_t), g_beacon_cfg.enable_bcast_twr,
                 g_beacon_cfg.val_id_mode);

          m_tx_buf[1] = 0x00; // Success
        }
        else
        {
          m_tx_buf[1] = 0x01; // Error size
        }

        m_tx_buf[0] = CMD_SET_BEACON_STR | 0x80;
        memset(&m_tx_buf[2], 0xFF, BUF_LEN - 2);
        break;
      }

      case CMD_SET_BEACON_DYNAMIC:
      {
        // Header length is 7 bytes (type, crc, add, len)
        // Data starts at m_rx_copy[7]
        uint8_t count = m_rx_copy[7];
        uint8_t *p_data = &m_rx_copy[8];

        // KHÔNG dùng memset ở đây để giữ lại các field ID/Major/Minor/Config
        // cũ. Chỉ cập nhật những field có trong danh sách p_data.
        // memset(&g_beacon_cfg, 0, sizeof(beacon_cfg_t));

        for (uint8_t i = 0; i < count; i++)
        {
          uint16_t id = p_data[0] | (p_data[1] << 8);
          uint32_t val = (uint32_t)p_data[2] | ((uint32_t)p_data[3] << 8) |
                         ((uint32_t)p_data[4] << 16) |
                         ((uint32_t)p_data[5] << 24);
          p_data += 6;

          // Update g_beacon_cfg based on ID (matching MAJOR_ definitions)
          switch (id)
          {
          case MAJOR_TIMER_SEND_TAG_MOTION:
            g_beacon_cfg.val_motion = val;
            break;
          case MAJOR_TIMER_SEND_TAG_STAND:
            g_beacon_cfg.val_stand = val;
            break;
          case MAJOR_TIMER_SEND_TAG_SLEEP_MODE_1:
            g_beacon_cfg.val_sleep1 = val;
            break;
          case MAJOR_TIMER_SEND_TAG_SLEEP_MODE_2:
            g_beacon_cfg.val_sleep2 = val;
            break;
          case MAJOR_TIMER_SEND_TAG_SLEEP_MODE_3:
            g_beacon_cfg.val_sleep3 = val;
            break;
          case MAJOR_TIMER_SLEEP_MODE_1:
            g_beacon_cfg.val_mode1 = val;
            break;
          case MAJOR_TIMER_SLEEP_MODE_2:
            g_beacon_cfg.val_mode2 = val;
            break;
          case MAJOR_TIMER_SLEEP_MODE_3:
            g_beacon_cfg.val_mode3 = val;
            break;

          case MAJOR_BATT_UPDATE_DEFAULT:
            g_beacon_cfg.val_batt_default = val;
            break;
          case MAJOR_BATT_UPDATE_HIGH:
            g_beacon_cfg.val_batt_high = val;
            break;
          case MAJOR_BATT_INCREASE:
            g_beacon_cfg.val_batt_inc = val;
            break;
          case MAJOR_BATT_DECREASE:
            g_beacon_cfg.val_batt_dec = val;
            break;

          case MAJOR_CONFIG_UWB_CHAN:
            g_beacon_cfg.uwb_chan = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_PLEN:
            g_beacon_cfg.uwb_plen = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_PAC:
            g_beacon_cfg.uwb_pac = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_TXCODE:
            g_beacon_cfg.uwb_txcode = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_RXCODE:
            g_beacon_cfg.uwb_rxcode = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_SFDTYPE:
            g_beacon_cfg.uwb_sfdtype = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_DATARATE:
            g_beacon_cfg.uwb_datarate = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_PHRMODE:
            g_beacon_cfg.uwb_phrmode = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_PHRRATE:
            g_beacon_cfg.uwb_phrrate = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_SFDTO:
            g_beacon_cfg.uwb_sfdto = (uint16_t)val;
            break;
          case MAJOR_CONFIG_UWB_STSMODE:
            g_beacon_cfg.uwb_stsmode = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_STSLEN:
            g_beacon_cfg.uwb_stslen = (uint8_t)val;
            break;
          case MAJOR_CONFIG_UWB_PDOA:
            g_beacon_cfg.uwb_pdoa = (uint8_t)val;
            break;

          case MAJOR_STATE_DEFAULT:
            if (val)
              g_beacon_cfg.val_id_mode = 0;
            break;
          case MAJOR_STATE_TX:
            if (val)
              g_beacon_cfg.val_id_mode = 1;
            break;
          case MAJOR_STATE_OFF_UWB:
            if (val)
              g_beacon_cfg.val_id_mode = 2;
            break;
          case MAJOR_STATE_SOS:
            if (val)
              g_beacon_cfg.val_id_mode = 3;
            break;
          case MAJOR_STATE_IDENTIFY:
            if (val)
              g_beacon_cfg.val_id_mode = 4;
            break;
          case MAJOR_STATE_TWR:
            if (val)
              g_beacon_cfg.val_id_mode = 5;
            break;
          case MAJOR_STATE_RESET:
            if (val)
              g_beacon_cfg.val_id_mode = 6;
            break;
          case MAJOR_STATE_AIRPLAN:
            if (val)
              g_beacon_cfg.val_id_mode = 7;
            break;
          case MAJOR_STATE_MOTION:
            if (val)
              g_beacon_cfg.val_id_mode = 8;
            break;
          case MAJOR_STATE_NO_CHANGE:
            if (val)
              g_beacon_cfg.val_id_mode = 0xFE;
            break;

          case MAJOR_REQUEST_TIMER:
            if (val)
            {
              g_beacon_cfg.val_request = 1;
            }
            break;
          case MAJOR_REQUEST_CONFIG:
            if (val)
            {
              g_beacon_cfg.val_request = 2;
            }
            break;
          case MAJOR_REQUEST_STATE:
            if (val)
            {
              g_beacon_cfg.val_request = 3;
            }
            break;
          case MAJOR_REQUEST_ID_TAG:
            if (val)
            {
              g_beacon_cfg.val_request = 4;
            }
            break;

          case MAJOR_ID_CHANGE_FLAG:
            g_beacon_cfg.val_id_change = (uint8_t)val;
            break;
          case MAJOR_ID_LAST_BYTE1:
            g_beacon_cfg.val_id_last[0] = (uint8_t)val;
            break;
          case MAJOR_ID_LAST_BYTE2:
            g_beacon_cfg.val_id_last[1] = (uint8_t)val;
            break;
          case MAJOR_ID_LAST_BYTE3:
            g_beacon_cfg.val_id_last[2] = (uint8_t)val;
            break;
          case MAJOR_ID_LAST_BYTE4:
            g_beacon_cfg.val_id_last[3] = (uint8_t)val;
            break;
          case MAJOR_ID_LAST_BYTE5:
            g_beacon_cfg.val_id_last[4] = (uint8_t)val;
            break;
          case MAJOR_ID_CHANGE_BYTE1:
            g_beacon_cfg.val_id_new[0] = (uint8_t)val;
            break;
          case MAJOR_ID_CHANGE_BYTE2:
            g_beacon_cfg.val_id_new[1] = (uint8_t)val;
            break;
          case MAJOR_ID_CHANGE_BYTE3:
            g_beacon_cfg.val_id_new[2] = (uint8_t)val;
            break;
          case MAJOR_ID_CHANGE_BYTE4:
            g_beacon_cfg.val_id_new[3] = (uint8_t)val;
            break;
          case MAJOR_ID_CHANGE_BYTE5:
            g_beacon_cfg.val_id_new[4] = (uint8_t)val;
            break;
          case MAJOR_MAC_ADDRESS_BYTE1:
            g_beacon_cfg.mac_address[0] = (uint8_t)val;
            break;
          case MAJOR_MAC_ADDRESS_BYTE2:
            g_beacon_cfg.mac_address[1] = (uint8_t)val;
            break;
          case MAJOR_MAC_ADDRESS_BYTE3:
            g_beacon_cfg.mac_address[2] = (uint8_t)val;
            break;
          case MAJOR_MAC_ADDRESS_BYTE4:
            g_beacon_cfg.mac_address[3] = (uint8_t)val;
            break;
          case MAJOR_MAC_ADDRESS_BYTE5:
            g_beacon_cfg.mac_address[4] = (uint8_t)val;
            break;
          case MAJOR_APPLY_CONFIG_TO_BASE:
            g_beacon_cfg.apply_config_to_base = (uint8_t)val;
            break;
          case MAJOR_BROADCAST_FLAG:
            g_beacon_cfg.broadcast = (uint8_t)val;
            break;
          case MAJOR_ENABLE_BCAST_TWR:
          {
            bool was_bcast_twr_enabled =
                (g_beacon_cfg.enable_bcast_twr != 0);
            g_beacon_cfg.enable_bcast_twr = (uint8_t)val;
            printf("[nRF] SPI: Received MAJOR_ENABLE_BCAST_TWR val=%d\n",
                   (int)val);
            if (val)
            {
              flag_init_bcast_twr = true; // Init queue trong main loop
            }
            else if (was_bcast_twr_enabled)
            {
              bcast_twr_disable();
              ducthang_ble_stop_all_timers();
              restore_dw3000_to_tdoa();
            }
            break;
          }
          case MAJOR_SETTING_CHARGE_TX:
            g_beacon_cfg.charge_tx = (uint8_t)val;
            break;
          case MAJOR_SETTING_OTA:
            g_beacon_cfg.ota_enable = (uint8_t)val;
            break;
          case MAJOR_SETTING_SYS_CONFIG_DEFAULT:
            g_beacon_cfg.sys_config_default = (uint8_t)val;
            break;
          case MAJOR_SETTING_SLEEP:
            g_beacon_cfg.sleep_enable = (uint8_t)val;
            break;
          }
        }

        // printf("[nRF] Received IDs -> Last: %02X%02X%02X%02X%02X, New: "
        //        "%02X%02X%02X%02X%02X, Change: %u\n",
        //        g_beacon_cfg.val_id_last[0], g_beacon_cfg.val_id_last[1],
        //        g_beacon_cfg.val_id_last[2], g_beacon_cfg.val_id_last[3],
        //        g_beacon_cfg.val_id_last[4], g_beacon_cfg.val_id_new[0],
        //        g_beacon_cfg.val_id_new[1], g_beacon_cfg.val_id_new[2],
        //        g_beacon_cfg.val_id_new[3], g_beacon_cfg.val_id_new[4],
        //        g_beacon_cfg.val_id_change);

        m_tx_buf[0] = CMD_SET_BEACON_DYNAMIC | 0x80;
        m_tx_buf[1] = 0x00;
        memset(&m_tx_buf[2], 0xFF, BUF_LEN - 2);

        // Bắt đầu phát cấu hình mới
        cache_pending_dwt_config();
        start_config_advertising();

        // Sau khi update dynamic config, sync ID ngay
        update_ducthang_ids();
        break;
      }

      case CMD_DO_DS_TWR:
      {
        // printf("[CMD] DO_DS_TWR\n");
        mode_run = mode_twr; // chuyển sang chế độ đo khoảng cách
        dwt_setrxtimeout(
            RESP_RX_TIMEOUT_UUS_OFFSET); // Sử dụng timeout ngắn cho TWR

        // has_tag_frame_ready = false;

        if (spis_rx_len < 5)
        {
          // printf("[ERR] Thiếu target_id\n");
          m_tx_buf[0] = CMD_DO_DS_TWR | 0x80;
          m_tx_buf[1] = 0xFE; // lỗi thiếu dữ liệu
          m_tx_buf[2] = 0xFF; // loại frame không xác định
          m_tx_buf[3] = 0x00; // không có payload
          break;
        }

        // Parse 4 byte target Base ID từ payload (DS-TWR base-base)
        ds_twr_target_base_id = ((uint32_t)m_rx_copy[1]) |
                                ((uint32_t)m_rx_copy[2] << 8) |
                                ((uint32_t)m_rx_copy[3] << 16) |
                                ((uint32_t)m_rx_copy[4] << 24);

        // printf(">> DS-TWR target base = 0x%08lX (%lu)\n",
        //        ds_twr_target_base_id, ds_twr_target_base_id);
        flag_send_start_twr = true; // Đánh dấu cần gửi gói Poll
        // Gọi hàm thực hiện đo khoảng cách
        // send_start_twr(target_id);
        // Chuẩn bị phản hồi OK
        m_tx_buf[0] = CMD_DO_DS_TWR | 0x80;
        m_tx_buf[1] = OK_STATUS;                 // trạng thái OK
        m_tx_buf[2] = 0x00;                      // không có frame data kèm theo
        m_tx_buf[3] = 0x00;                      // không có payload
        memset(&m_tx_buf[4], 0xFF, BUF_LEN - 4); // clear phần dư
        break;
      }

      case CMD_GET_FRAME_DW:
      {

        // has_tag_frame_ready = false;
        //  printf("[CMD] GET_FRAME_DW\n");
        //   memcpy(&m_tx_buf[2], &last_dw_frame, sizeof(dw_dataframe_t));
        //   m_tx_buf[0] = CMD_GET_FRAME_DW | 0x80;
        //   m_tx_buf[1] = 0x00;
        break;
      }

      case CMD_SEND_TX_OFFSET:
      {
        // has_tag_frame_ready = false;
        //  printf("[CMD] CMD_SEND_TX_OFFSET\n");
        mode_run = mode_offset_tx;
        // Kiểm tra độ dài payload
        // if (spis_rx_len < sizeof(dw_dataframe_t) + 1)
        // {
        //   printf("[ERR] Payload OFFSET thiếu dữ liệu (%d bytes)\n",
        //   spis_rx_len); m_tx_buf[0] = CMD_SEND_TX_OFFSET | 0x80;
        //   m_tx_buf[1] = 0xFE; // lỗi độ dài memset(&m_tx_buf[2], 0xFF,
        //   BUF_LEN - 2); break;
        // }

        // Parse frame từ SPI buffer (bỏ byte đầu tiên là CMD)
        dw_dataframe_t *offset_frame = (dw_dataframe_t *)&m_rx_copy[1];

        // In thông tin frame để debug
        // printf("📤 Gửi OFFSET:\n");
        // printf("  Src       : 0x%06lX\n", offset_frame->Src);
        // printf("  Des       : 0x%06lX\n", offset_frame->Des);
        // printf("  Cmd       : %u\n", offset_frame->Cmd);
        // printf("  Packet_ID : %lu\n", offset_frame->Packet_Id);
        // printf("  Timestamp : %02X %02X %02X %02X %02X\n",
        //        offset_frame->Data.SYNC.Ts[0],
        //        offset_frame->Data.SYNC.Ts[1],
        //        offset_frame->Data.SYNC.Ts[2],
        //        offset_frame->Data.SYNC.Ts[3],
        //        offset_frame->Data.SYNC.Ts[4]);

        // Gửi gói tin ra DW3000
        int ret =
            dw3000_tx_frame((uint8_t *)offset_frame, sizeof(dw_dataframe_t));

        if (ret != DWT_SUCCESS)
        {
          // printf("❌ Gửi OFFSET qua DW3000 thất bại!\n");
          m_tx_buf[0] = CMD_SEND_TX_OFFSET | 0x80;
          m_tx_buf[1] = 0x01; // lỗi gửi
          memset(&m_tx_buf[2], 0xFF, BUF_LEN - 2);
          break;
        }

        // printf("✅ OFFSET đã được gửi qua DW3000\n");

        // Trả về gói OFFSET đã gửi cho ESP32
        m_tx_buf[0] = CMD_GET_OFFSET_FRAME | 0x80;
        m_tx_buf[1] = OK_STATUS;
        m_tx_buf[2] = FRAME_TYPE_OFFSET;
        m_tx_buf[3] = sizeof(dw_dataframe_t);
        memcpy(&m_tx_buf[4], offset_frame, sizeof(dw_dataframe_t));

        // Cài lại buffer SPI (Đã được xử lý ở next_spi_ready)

        // Có thể bật IRQ nếu cần báo ESP32
        TAG_IRQ_SET();
        TAG_IRQ_CLR();

        break;
      }

      case CMD_GET_BCAST_TWR_RESULT:
      {
        // Poll queue BSS-TWR - U7 gọi mỗi 500ms
        twr_queue_entry_t entry;
        if (twr_queue_pop(&entry))
        {
          dw_twr_result_t res;
          memcpy(res.tag_id, entry.tag_id, 5);
          res.distance_mm = entry.distance_mm;
          m_tx_buf[0] = CMD_GET_BCAST_TWR_RESULT | 0x80;
          m_tx_buf[1] = OK_STATUS;
          m_tx_buf[2] = FRAME_TYPE_BCAST_TWR_RESULT;
          m_tx_buf[3] = sizeof(dw_twr_result_t);
          memcpy(&m_tx_buf[4], &res, sizeof(dw_twr_result_t));
          memset(&m_tx_buf[4 + sizeof(dw_twr_result_t)], 0xFF,
                 BUF_LEN - 4 - sizeof(dw_twr_result_t));
        }
        else
        {
          // Queue rỗng
          m_tx_buf[0] = CMD_GET_BCAST_TWR_RESULT | 0x80;
          m_tx_buf[1] = 0xFF; // No data
          m_tx_buf[2] = 0;
          m_tx_buf[3] = 0;
          memset(&m_tx_buf[4], 0xFF, BUF_LEN - 4);
        }
        break;
      }

      case CMD_GET_SS_TWR_DISTANCE:
      {
        if (g_beacon_cfg.enable_bcast_twr)
        {
          // Khi hoạt động ở Bcast TWR, không được phép hỏi chéo DS-TWR
          m_tx_buf[0] = CMD_GET_SS_TWR_DISTANCE | 0x80;
          m_tx_buf[1] = 0xFF; // Báo hiệu chế độ này đang bị tắt
          memset(&m_tx_buf[2], 0xFF, BUF_LEN - 2);
        }
        else
        {
          float distance = ss_twr_ducthang_get_distance();
          m_tx_buf[0] = CMD_GET_SS_TWR_DISTANCE | 0x80;
          m_tx_buf[1] = OK_STATUS;
          m_tx_buf[2] = 0x00; // No frame type
          m_tx_buf[3] = 4;    // 4 byte float
          memcpy(&m_tx_buf[4], &distance, 4);
          memset(&m_tx_buf[8], 0xFF, BUF_LEN - 8);
        }
        break;
      }

      case CMD_GET_FRAGMENT_STATUS:
      {
        m_tx_buf[0] = CMD_GET_FRAGMENT_STATUS | 0x80;
        m_tx_buf[1] = OK_STATUS;
        m_tx_buf[2] = 0x00; // No frame type
        m_tx_buf[3] = 8;    // 8 bytes (uint64_t)
        memcpy(&m_tx_buf[4], &fragment_status, 8);
        memset(&m_tx_buf[12], 0xFF, BUF_LEN - 12);
        break;
      }

      case CMD_GET_REQUEST_DATA:
      {
        m_tx_buf[0] = CMD_GET_REQUEST_DATA | 0x80;
        m_tx_buf[1] = OK_STATUS;
        m_tx_buf[2] = FRAME_TYPE_REQUEST;
        m_tx_buf[3] = sizeof(request_data_t);
        memcpy(&m_tx_buf[4], &g_request_data, sizeof(request_data_t));
        memset(&m_tx_buf[4 + sizeof(request_data_t)], 0xFF,
               BUF_LEN - 4 - sizeof(request_data_t));
        break;
      }

      /*─── OTA Commands ─────────────────────────────────*/
      case CMD_OTA_ENTER:
      {
        /* Dừng Broadcast TWR trước khi bắt đầu OTA để OTA độc quyền SPI ACK */
        if (g_beacon_cfg.enable_bcast_twr)
        {
          printf("[CMD] OTA_ENTER: clean exit BSS-TWR before OTA\n");
          bcast_twr_disable();
          flag_start_ducthang_twr = false;
          g_beacon_cfg.enable_bcast_twr = 0;
          ducthang_ble_stop_all_timers();
          ss_twr_ducthang_cleanup();
        }

        /* Dừng toàn bộ DW3000 RX trước khi bắt đầu OTA */
        dwt_forcetrxoff();
        enable_polling_rx = false;

        ota_handle_enter(m_rx_copy, spis_rx_len, m_tx_buf);
        printf("[CMD] OTA_ENTER → state=%d\n", ota_isp_get_state());
        break;
      }

      case CMD_OTA_DATA:
      {
        ota_handle_data(m_rx_copy, spis_rx_len, m_tx_buf);
        break;
      }

      case CMD_OTA_END:
      {
        ota_handle_end(m_tx_buf);
        printf("[CMD] OTA_END → state=%d\n", ota_isp_get_state());
        break;
      }

      case CMD_OTA_ABORT:
      {
        ota_handle_abort(m_tx_buf);
        printf("[CMD] OTA_ABORT → back to IDLE\n");
        /* Khôi phục DW3000 */
        begin_dw3000();
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        enable_polling_rx = true;
        break;
      }

      case CMD_OTA_STATUS:
      {
        ota_handle_status(m_tx_buf);
        break;
      }

      default:
      {
        if (cmd != 0x00)
        {
          printf("[CMD] Unknown 0x%02X len %d\n", cmd, spis_rx_len);
        }
        // m_tx_buf[0] = cmd | 0x80;
        // m_tx_buf[1] = 0xFF;
        // memset(&m_tx_buf[2], 0xFF, BUF_LEN - 2);
        break;
      }
      }

    next_spi_ready:
    {
      ret_code_t err =
          nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN);
      if (err != NRF_SUCCESS)
      {
        printf("[SPI] Buffers set error: %lu\n", err);
        // Thay vì dùng APP_ERROR_CHECK (gây reset), ta delay một chút rồi thử
        // lại hoặc bỏ qua
        nrf_delay_ms(5);
        nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN);
      }
    }
    }
  }
}
void spi_send_request_done(void) { flag_request_data_ready = true; }

void spi_send_ack_status_update(void) { flag_ack_status_ready = true; }