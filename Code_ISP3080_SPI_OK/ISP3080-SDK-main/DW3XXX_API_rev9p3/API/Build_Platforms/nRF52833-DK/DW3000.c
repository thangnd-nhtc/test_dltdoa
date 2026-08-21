#include "DW3000.h"
#include "main.h"
#include <math.h>
#include <stdio.h>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// bool has_tag_frame_ready = false;
volatile uint64_t last_rx_timestamp = 0;
server_dataframe_t server_frame;
dw_dataframe_t last_dw_frame;
twr_local_timestamp_t twr_ts_a = {0};
twr_remote_timestamp_t twr_ts_b = {0};
tag_cache_entry_t tag_cache[MAX_TAG_TRACKED];
uint8_t tag_cache_index = 0;

dwt_config_t config = {.chan = 9,
                       .txPreambLength = DWT_PLEN_2048,
                       .rxPAC = DWT_PAC16,
                       .txCode = 9,
                       .rxCode = 9,
                       .sfdType = 3,
                       .dataRate = DWT_BR_850K,
                       .phrMode = DWT_PHRMODE_STD,
                       .phrRate = DWT_PHRRATE_STD,
                       .sfdTO = (2041),
                       //.sfdTO = (2048 + 1 - 16 + 8),
                       .stsMode = DWT_STS_MODE_OFF,
                       .stsLength = DWT_STS_LEN_256,
                       .pdoaMode = DWT_PDOA_M0};

// Hàm in thông tin debug
void test_run_info(unsigned char *data) {
  printf("%s", data); // hoặc SEGGER_RTT_WriteString(0, (char*)data);
}

// Hàm khởi tạo DW3000
uint8_t begin_dw3000(void) {
  // printf("[DW] Step 1: Set SPI fastrate\r\n");
  // port_set_dw_ic_spi_fastrate();
  // nrf52840_dk_spi_init(); // SPI cho DW3000
  // printf("[DW] Step 2: Reset DW3000\r\n");
  // reset_DWIC();
  // port_set_dw_ic_spi_fastrate();

  // printf("[DW] Step 3: Probe DW3000\r\n");
  if (dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf) != DWT_SUCCESS) {
    // printf("[DW] ERROR: dwt_probe() failed\r\n");
    return DW_ERR_PROBE;
  }

  // printf("[DW] Step 5: dwt_initialise()\r\n");
  if (dwt_initialise(DWT_DW_INIT) != DWT_SUCCESS) {
    printf("[DW] ERROR: dwt_initialise() failed\r\n");
    return DW_ERR_INIT;
  }

  // printf("[DW] Step 4: Wait for IDLE_RC\r\n");
  // uint32_t timeout = 10000;
  uint32_t timeout = 1000000; // Khoảng 1s
  while (!dwt_checkidlerc()) {
    if (--timeout == 0) {
      printf("[DW] ERROR: Timeout waiting for IDLE_RC\r\n");
      return DW_ERR_PROBE;
    }
    deca_sleep(1);
  }

  // printf("[DW] Step 09: LED blink\r\n");
  // dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
  dwt_configureframefilter(DWT_FF_DISABLE,
                           DWT_FF_DISABLE); // Disable frame filtering

  // printf("[DW] Step 6: dwt_configure()\r\n");
  if (dwt_configure((dwt_config_t *)&config) != DWT_SUCCESS) {
    printf("[DW] ERROR: dwt_configure() failed\r\n");
    return DW_ERR_CONFIG;
  }

  /* Enable IC diagnostic calculation and logging */
  dwt_configciadiag(1);

  // printf("[DW] Step 7: TX power, LNA/PA\r\n");
  dwt_configuretxrf(&txconfig_options_ch9);

  // printf("[DW] Step 8: Set delays and RX timeout\r\n");
  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);

  dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

  dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
  dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);

  // dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
  //  dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
  //  Chú ý cấu hình Sau khi hoàn tất TWR (Initiator hoặc Responder):
  //  dwt_setrxtimeout(0);            // Tắt timeout
  //  dwt_setrxaftertxdelay(0);       // Tắt delay RX
  //  dwt_rxenable(DWT_START_RX_IMMEDIATE); // Vào lại RX mode tự do

  dwt_config_ostr_mode(1, 33); // dung sync

  //  printf(">> Cau hinh PHY:\n");
  //  printf("  - Channel       : %u\n", config.chan);
  //  printf("  - Preamble Len  : %u\n", config.txPreambLength);
  //  printf("  - PAC Size      : %u\n", config.rxPAC);
  //  printf("  - Preamble Code : TX=%u, RX=%u\n", config.txCode,
  //  config.rxCode); printf("  - SFD Type      : %u\n", config.sfdType);
  //  printf("  - Data rate     : %u\n", config.dataRate);
  //  printf("  - PHR Mode/Rate : mode=%u, rate=%u\n", config.phrMode,
  //  config.phrRate); printf("  - STS Mode      : %u\n", config.stsMode);
  //  printf("  - PDOA Mode     : %u\n", config.pdoaMode);
  //  printf("  - SFD Timeout   : %u\n", config.sfdTO);

  //  printf("[DW] DONE: DW3000 initialized OK\r\n");
  return 0; /* SUCCESS */
}

uint8_t read_rssi_from_dw3000(void) {
  dwt_rxdiag_t diag;
  // diag.ipatovAccumCount = 0; // Đảm bảo khởi tạo giá trị
  // diag.ipatovPower = 0; // Đảm bảo khởi tạo giá trị
  dwt_readdiagnostics(&diag);
  // printf("ipatovPower = %u, ipatovAccumCount = %u\n", diag.ipatovPower,
  // diag.ipatovAccumCount);

  if (diag.ipatovAccumCount == 0 || diag.ipatovPower == 0)
    return 0;

  float rssi_dbm =
      10.0f * log10f(((float)diag.ipatovPower / diag.ipatovAccumCount) *
                     131072.0f) -
      121.74f;
  int16_t rssi_q8_8 = (int16_t)(rssi_dbm * 256.0f);
  uint8_t rssi_u8 = (uint8_t)((rssi_q8_8 >> 8) + 128); // dBm -> uint8_t
  // printf("RSSI � %.2f dBm (Q8.8 = %d)\n", rssi_dbm, rssi_q8_8);
  return rssi_u8;
}

static bool is_valid_tdoa_tag_frame(const uint8_t *buf, uint16_t frame_len)
{
  if (frame_len < sizeof(tag_frame_t))
  {
    return false;
  }

  // Chặn frame SS-TWR chuẩn Decawave example trước khi cast sang tag_frame_t.
  // POLL: 41 88 xx CA DE 'W' 'A' 'V' 'E' E0
  // RESP: 41 88 xx CA DE 'V' 'E' 'W' 'A' E1
  if (frame_len >= 10 && buf[0] == 0x41 && buf[1] == 0x88 &&
      buf[3] == 0xCA && buf[4] == 0xDE)
  {
    if ((buf[5] == 'W' && buf[6] == 'A' && buf[7] == 'V' &&
         buf[8] == 'E' && buf[9] == 0xE0) ||
        (buf[5] == 'V' && buf[6] == 'E' && buf[7] == 'W' &&
         buf[8] == 'A' && buf[9] == 0xE1))
    {
      return false;
    }
  }

  const tag_frame_t *tag = (const tag_frame_t *)buf;

  if (tag->Cmd != Cmd_Tag)
  {
    return false;
  }

  // TDOA tag hợp lệ hiện chỉ dùng các type này. Type lạ thường là do parse nhầm
  // frame UWB khác loại.
  if (!(tag->Data.TAG.Type == Cmd_tag_nomal ||
        tag->Data.TAG.Type == Cmd_tag_sensor ||
        tag->Data.TAG.Type == Cmd_tag_solut ||
        tag->Data.TAG.Type == Cmd_tag_dps422 ||
        tag->Data.TAG.Type == cmd_custom_ducthang))
  {
    return false;
  }

  // TDOA tag thường gửi broadcast/0 hoặc đích danh base. Nếu bắt nhầm SS-TWR,
  // 4 byte đầu sẽ là MAC header 0x41 0x88 ... nên Des không khớp và bị drop.
  if (!(tag->Des == 0 || tag->Des == 0xFFFFFFFF || tag->Des == MY_DEVICE_ID))
  {
    return false;
  }

  return true;
}

// Hàm nhận gói tin từ DW3000
void poll_rx_once(void) {
  // if (mode_run == 0) // nếu đang ở chế độ TDOA
  // {
  // if (has_tag_frame_ready == true)
  // {
  //     return; // đã có frame, chờ ESP32 đọc
  // }

  uint16_t frame_len;
  uint32_t status_reg;

  // Bắt đầu nhận ngay lập tức
  if (dwt_rxenable(DWT_START_RX_IMMEDIATE) != DWT_SUCCESS) {
    printf("[DW] RX enable failed\n");
    return;
  }

  // Chờ có gói tin hoặc lỗi RX
  // waitforsysstatus(&status_reg, NULL,
  //                  (DWT_INT_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR), 0);
  waitforsysstatus(&status_reg, NULL,
                   DWT_INT_RXFCG_BIT_MASK | DWT_INT_RXFTO_BIT_MASK |
                       SYS_STATUS_ALL_RX_ERR,
                   0);

  if (status_reg & DWT_INT_RXFCG_BIT_MASK) {
    dwt_writesysstatuslo(DWT_INT_RXFCG_BIT_MASK); // clear cờ RX ok

    frame_len = dwt_getframelength(0);
    if (frame_len > sizeof(rx_buffer))
      return;

    dwt_readrxdata(rx_buffer, frame_len, 0);
    dw_dataframe_t *frame = (dw_dataframe_t *)rx_buffer;
    // print_dw_dataframe(frame);

    // Lấy timestamp và RSSI
    last_rx_timestamp = get_rx_timestamp_u64();
    uint8_t rssi = read_rssi_from_dw3000();

    // Phân loại gói tin dựa trên Cmd
    switch (frame->Cmd) {
    case Cmd_Tag:
      if (mode_run != mode_tdoa)
        break;

      if (!is_valid_tdoa_tag_frame(rx_buffer, frame_len)) {
        // Drop gói UWB có Cmd giống Tag nhưng app-header không đúng TDOA.
        // Trường hợp thường gặp: Base khác đang BSS-TWR, Base này RX nhầm rồi
        // parse thành CSV type 1/source lạ.
        break;
      }

      build_server_frame((tag_frame_t *)frame, last_rx_timestamp, rssi,
                         &server_frame);
      tdoa_uwb_received++;
      tdoa_queue_push(&server_frame);
      tdoa_prepare_spi_frame();
      tdoa_update_irq();
       //print_server_frame(&server_frame);
      break;

    case Cmd_start_twr:
      mode_run = mode_twr; // Chế độ TWR
      // Bắt đầu chuỗi đo khoảng cách DS-TWR
      twr_start_once_rx(frame, last_rx_timestamp);
      break;

    case Cmd_Pool:
      mode_run = mode_twr; // Chế độ TWR
      // Nhận Poll → gửi Resp (Cmd_Resp)
      twr_poll_once_rx(frame, last_rx_timestamp);
      break;

    case Cmd_Resp:
      mode_run = mode_twr; // Chế độ TWR
      // Nhận Resp → gửi Final (Cmd_Final)
      twr_resp_once_rx(frame, last_rx_timestamp);
      mode_run = mode_tdoa;
      break;

    case Cmd_Final:
      mode_run = mode_twr; // Chế độ TWR
      // Nhận Final → tính khoảng cách → gửi Distance
      twr_final_once_rx(frame, last_rx_timestamp);
      memcpy(&m_tx_buf[4], frame, sizeof(dw_dataframe_t));
      m_tx_buf[0] = CMD_GET_FRAME_DW | 0x80;
      m_tx_buf[1] = OK_STATUS;     // Trạng thái OK
      m_tx_buf[2] = FRAME_TYPE_DW; // loại gói tin TWR
      m_tx_buf[3] = sizeof(dw_dataframe_t);
      nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN);
      // print_dw_dataframe(frame);
      TAG_IRQ_SET(); // báo ESP32 có gói mới
      TAG_IRQ_CLR(); // báo ESP32 đã đọc
      //   Sau khi đo xong, quay về chế độ định vị TDoA
      mode_run = mode_tdoa;
      dwt_setrxtimeout(
          RESP_RX_TIMEOUT_UUS); // Khôi phục timeout mặc định sau khi đo xong
      break;

    case Cmd_Sync:
      // Xử lý SYNC nếu cần
      // printf("[DW] Nhận gói SYNC\n");
      break;

      // case Cmd_Offset:
      //     printf("[DW] Nhận gói OFFSET\n");

      //     // Ghi gói tin vào buffer trả về cho ESP32
      //     memcpy(&m_tx_buf[4], frame, sizeof(dw_dataframe_t));
      //     m_tx_buf[0] = CMD_GET_OFFSET_FRAME | 0x80; // hoặc
      //     CMD_GET_OFFSET_FRAME nếu bạn muốn tách riêng m_tx_buf[1] =
      //     OK_STATUS;                   // Trạng thái OK m_tx_buf[2] =
      //     FRAME_TYPE_OFFSET;           // loai goi tin Offset m_tx_buf[3] =
      //     sizeof(dw_dataframe_t);
      //     // Chuẩn bị phản hồi SPI
      //     nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN);

      //     // Báo ESP32 biết là có gói mới (IRQ)
      //     TAG_IRQ_SET();
      //     TAG_IRQ_CLR();
      //     break;

    case Cmd_Offset: {
      // printf("[DW] Nhận gói OFFSET\n");

      dw_dataframe_t frame_modified; // đưa khai báo ra ngoài để dùng được trong
                                     // cả if và else

      if (sizeof(dw_dataframe_t) <=
          sizeof(m_tx_buf) - 4) // kiểm tra an toàn buffer
      {
        dw_dataframe_t *f = (dw_dataframe_t *)frame;

        memcpy(&frame_modified, f, sizeof(dw_dataframe_t));

        // memcpy(frame_modified.Data.SYNC.Ts, &last_rx_timestamp, 5);
        // for (int i = 0; i < 5; i++)
        // {
        //     frame_modified.Data.SYNC.Ts[i] = (last_rx_timestamp >> (i * 8)) &
        //     0xFF;
        // }
        uint64_t tmp_ts = last_rx_timestamp;
        for (int i = 0; i < 5; i++) {
          frame_modified.Data.SYNC.Ts[i] = (tmp_ts >> (i * 8)) & 0xFF;
        }

        memcpy(&m_tx_buf[4], &frame_modified, sizeof(dw_dataframe_t));
      } else {
        // printf("[ERR] Buffer không đủ lớn để gửi OFFSET frame!\n");
        memset(&frame_modified, 0,
               sizeof(dw_dataframe_t)); // đề phòng print_dw_dataframe
      }

      // ✅ In log nội dung frame
      // print_dw_offset_frame(&frame_modified);

      // Header phản hồi SPI
      m_tx_buf[0] = CMD_SEND_TX_OFFSET | 0x80;
      m_tx_buf[1] = OK_STATUS;
      m_tx_buf[2] = FRAME_TYPE_OFFSET;
      m_tx_buf[3] = sizeof(dw_dataframe_t);

      APP_ERROR_CHECK(
          nrfx_spis_buffers_set(&spis, m_tx_buf, BUF_LEN, m_rx_buf, BUF_LEN));

      TAG_IRQ_SET();
      TAG_IRQ_CLR();
      mode_run = mode_tdoa;
      dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
      break;
    }

    default:
      // printf("[DW] Nhận gói không xác định Cmd = %d\n", frame->Cmd);
      break;
    }
  } else {
    dwt_writesysstatuslo(SYS_STATUS_ALL_RX_ERR);
    if (mode_run == mode_twr) {
      mode_run = mode_tdoa;
      dwt_setrxtimeout(
          RESP_RX_TIMEOUT_UUS); // Khôi phục timeout nếu đo thất bại
    }
    // printf("[DW] Lỗi RX hoặc timeout. SYS_STATUS: 0x%08lX\n", status_reg);
  }
}

// Hàm lấy ID gói tin tiếp theo
uint32_t get_next_packet_id(void) { return packet_id_counter++; }

// Các hàm xử lý TWR
// Gửi gói Poll, Resp, Final và tính khoảng cách
void send_start_twr(uint32_t target_id) {
  uint8_t tx_buffer[SIZE_OF_DATAFRAME + 2] = {0};

  dwt_forcetrxoff(); // Dừng bất kỳ TX/RX đang chạy
  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);
  // Sleep(10);

  dw_dataframe_t frame = {0};
  frame.Cmd = Cmd_start_twr;
  frame.Src = MY_DEVICE_ID;
  frame.Des = target_id;
  frame.TypeDev = 0xA5; // hoặc 0xBA tùy chọn định danh thiết bị
  frame.Packet_Id = packet_id_counter++;

  memcpy(tx_buffer, &frame, SIZE_OF_DATAFRAME);

  // uint64_t rx_ts = 0;
  // dwt_readsystime((uint8_t *)&rx_ts); // Lưu ý: dữ liệu là little endian
  // uint32_t tx_time = (rx_ts + TX_ANT_DLY * UUS_TO_DWT_TIME) >> 8;
  // dwt_setdelayedtrxtime(tx_time);

  dwt_writetxdata(SIZE_OF_DATAFRAME + 2, tx_buffer, 0);
  dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2, 0, 0);

  if (dwt_starttx(DWT_START_TX_IMMEDIATE) != DWT_SUCCESS) {
    // printf("Gui goi Cmd_start_twr den 0x%06X that bai\n", target_id);
    return;
  }

  // printf("Da gui Cmd_start_twr tu 0x%06X den 0x%06X (Packet_Id=%lu)\n",
  //        MY_DEVICE_ID, target_id, frame.Packet_Id);

  waitforsysstatus(NULL, NULL, DWT_INT_TXFRS_BIT_MASK, 0);
  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);

  // ✅ Sau khi gửi xong, bật lại RX
  dwt_forcetrxoff();
  if (dwt_rxenable(DWT_START_RX_IMMEDIATE) != DWT_SUCCESS) {
    // printf("Khong the bat lai che do RX\n");
  } else {
    // printf("Da bat lai RX, cho Cmd_Pool\n");
  }
}

void twr_start_once_rx(dw_dataframe_t *frame, uint64_t rx_ts_start) {
  // Kiểm tra nếu gói tin này không gửi đến mình → bỏ qua
  if (frame->Des != MY_DEVICE_ID) {
    // printf("Cmd_start_twr khong gui den thiet bi nay. Bo qua.\n");
    return;
  }

  uint8_t tx_buffer[SIZE_OF_DATAFRAME + 2] = {0};

  dwt_forcetrxoff();
  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);
  // Sleep(10);
  uint32_t tx_time = (rx_ts_start + TX_ANT_DLY * UUS_TO_DWT_TIME) >> 8;
  dwt_setdelayedtrxtime(tx_time);

  // Chuẩn bị frame phản hồi (Cmd_Pool)
  dw_dataframe_t resp = *frame;
  resp.Cmd = Cmd_Pool;
  resp.Src = MY_DEVICE_ID;
  resp.Des = frame->Src;
  resp.Packet_Id++;

  memcpy(tx_buffer, &resp, SIZE_OF_DATAFRAME);
  dwt_writetxdata(SIZE_OF_DATAFRAME + 2, tx_buffer, 0);
  dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2, 0, 1);

  if (dwt_starttx(DWT_START_TX_IMMEDIATE) != DWT_SUCCESS) {
    // printf("twr_start_once_rx() → Gui Cmd_Pool that bai\n");
    return;
  }

  // printf("twr_start_once_rx() → Gui Cmd_Pool den 0x%06X\n", resp.Des);

  // ⏳ Chờ gửi xong
  waitforsysstatus(NULL, NULL, DWT_INT_TXFRS_BIT_MASK, 0);
  // timestamp gửi
  twr_ts_b.poll_tx = get_tx_timestamp_u64();

  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);
  // ✅ Sau khi gửi xong, bật lại RX
  dwt_forcetrxoff();
  if (dwt_rxenable(DWT_START_RX_IMMEDIATE) != DWT_SUCCESS) {
    // printf("Khong the bat lai che do RX\n");
  } else {
    // printf("Da bat lai RX, cho Cmd_Resp\n");
  }
}

void twr_poll_once_rx(dw_dataframe_t *frame, uint64_t rx_ts) {
  // Kiểm tra nếu gói tin này không gửi đến thiết bị mình → bỏ qua
  if (frame->Des != MY_DEVICE_ID) {
    // printf("Cmd_Pool khong gui den thiet bi nay. Bo qua.\n");
    return;
  }

  // printf("Nhan Cmd_Pool tu 0x%06X → Gui Cmd_Resp\n", frame->Src);

  // ✅ Lưu timestamp poll_rx tại thiết bị A (cục bộ)
  twr_ts_a.poll_rx = rx_ts;

  uint8_t tx_buffer[SIZE_OF_DATAFRAME + 2] = {0};

  dwt_forcetrxoff();
  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);
  // Sleep(10);
  //  Tính thời điểm gửi phản hồi (delayed TX)
  uint32_t tx_time = (rx_ts + TX_ANT_DLY * UUS_TO_DWT_TIME) >> 8;
  dwt_setdelayedtrxtime(tx_time);

  // Chuẩn bị frame phản hồi Resp
  dw_dataframe_t resp = *frame;
  resp.Cmd = Cmd_Resp;
  resp.Src = MY_DEVICE_ID;
  resp.Des = frame->Src;

  // Không cần lưu timestamp vào frame ở đây (A giữ nội bộ)

  memcpy(tx_buffer, &resp, SIZE_OF_DATAFRAME);
  dwt_writetxdata(SIZE_OF_DATAFRAME + 2, tx_buffer, 0);
  dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2, 0, 1);

  if (dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) !=
      DWT_SUCCESS) {
    // printf("twr_poll_once_rx() → Gui Cmd_Resp that bai\n");
    return;
  }

  // Chờ TX xong rồi mới lấy timestamp gửi
  waitforsysstatus(NULL, NULL, DWT_INT_TXFRS_BIT_MASK, 0);

  // ✅ Lưu timestamp resp_tx tại A (sau khi TX xong)
  twr_ts_a.resp_tx = get_tx_timestamp_u64();

  // printf("twr_poll_once_rx() → Gui Cmd_Resp den 0x%06X\n", resp.Des);
  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);
  // ✅ Sau khi gửi xong, bật lại RX
  dwt_forcetrxoff();
  if (dwt_rxenable(DWT_START_RX_IMMEDIATE) != DWT_SUCCESS) {
    // printf("Khong the bat lai che do RX\n");
  } else {
    // printf("Da bat lai RX, cho Cmd_Final\n");
  }
}

void twr_resp_once_rx(dw_dataframe_t *frame, uint64_t rx_ts) {
  if (frame->Des != MY_DEVICE_ID) {
    // printf("Cmd_Resp khong gui den thiet bi nay. Bo qua.\n");
    return;
  }

  // printf("Nhan Cmd_Resp tu 0x%06X → Gui Cmd_Final\n", frame->Src);

  uint8_t tx_buffer[SIZE_OF_DATAFRAME + 2] = {0};
  dwt_forcetrxoff();
  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);
  // Sleep(10);
  //  Tính thời điểm gửi Final
  uint32_t tx_time = (rx_ts + TX_ANT_DLY * UUS_TO_DWT_TIME) >> 8;
  dwt_setdelayedtrxtime(tx_time);
  uint64_t final_tx_ts =
      (((uint64_t)(tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

  twr_ts_b.resp_rx = rx_ts;
  twr_ts_b.final_tx = final_tx_ts;

  // ✅ Tạo frame Final
  dw_dataframe_t final = *frame;
  final.Cmd = Cmd_Final;
  final.Src = MY_DEVICE_ID;
  final.Des = frame->Src;

  // ✅ Gán timestamp từ biến global twr_ts_b vào gói tin
  tdoa_set_timestamp_u64(twr_ts_b.poll_tx,
                         final.Data.DS_TWR.TIME_STAMP.poll_tx);
  tdoa_set_timestamp_u64(twr_ts_b.resp_rx,
                         final.Data.DS_TWR.TIME_STAMP.resp_rx);
  tdoa_set_timestamp_u64(twr_ts_b.final_tx,
                         final.Data.DS_TWR.TIME_STAMP.final_tx);

  memcpy(tx_buffer, &final, SIZE_OF_DATAFRAME);
  dwt_writetxdata(SIZE_OF_DATAFRAME + 2, tx_buffer, 0);
  dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2, 0, 1);

  if (dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) !=
      DWT_SUCCESS) {
    // printf("twr_resp_once_rx() → Gui Cmd_Final that bai\n");
    return;
  }

  // printf("twr_resp_once_rx() → Gui Cmd_Final den 0x%06X\n", final.Des);

  waitforsysstatus(NULL, NULL, DWT_INT_TXFRS_BIT_MASK, 0);
  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);

  dwt_forcetrxoff();
  if (dwt_rxenable(DWT_START_RX_IMMEDIATE) != DWT_SUCCESS) {
    // printf("Khong the bat lai RX\n");
  } else {
    // printf("Da bat lai RX, cho Cmd_Final\n");
  }
}

void twr_final_once_rx(dw_dataframe_t *frame, uint64_t rx_ts) {
  if (frame->Des != MY_DEVICE_ID) {
    // printf("Cmd_Final khong danh cho thiet bi nay. Bo qua.\n");
    return;
  }

  // printf("Nhan Cmd_Final tu 0x%06X → Bat dau tinh khoang cach\n",
  // frame->Src);

  // ⏱️ Lưu final_rx (cục bộ tại A)
  twr_ts_a.final_rx = rx_ts;

  // ⏱️ Lấy các timestamp phía B từ gói Cmd_Final
  uint64_t poll_tx_b = tdoa_get_timestamp_u64(
      frame->Data.DS_TWR.TIME_STAMP.poll_tx); // B gửi Poll
  uint64_t resp_rx_b = tdoa_get_timestamp_u64(
      frame->Data.DS_TWR.TIME_STAMP.resp_rx); // B nhận Resp
  uint64_t final_tx_b = tdoa_get_timestamp_u64(
      frame->Data.DS_TWR.TIME_STAMP.final_tx); // B gửi Final

  // ⏱️ Lấy các timestamp phía A từ biến global
  uint64_t poll_rx_a = twr_ts_a.poll_rx;   // A nhận Poll
  uint64_t resp_tx_a = twr_ts_a.resp_tx;   // A gửi Resp
  uint64_t final_rx_a = twr_ts_a.final_rx; // A nhận Final

  // 📜 In toàn bộ timestamp để kiểm tra thủ công
  // printf("🔍 Timestamp:\n");
  // printf("  poll_tx_b  = %llu\n", poll_tx_b);
  // printf("  resp_rx_b  = %llu\n", resp_rx_b);
  // printf("  final_tx_b = %llu\n", final_tx_b);
  // printf("  poll_rx_a  = %llu\n", poll_rx_a);
  // printf("  resp_tx_a  = %llu\n", resp_tx_a);
  // printf("  final_rx_a = %llu\n", final_rx_a);

  // 🧠 Tính toán theo công thức DS-TWR (3 message)
  double Ra = (double)((resp_rx_b - poll_tx_b) & 0xFFFFFFFFUL);
  double Rb = (double)((final_rx_a - resp_tx_a) & 0xFFFFFFFFUL);
  double Da = (double)((final_tx_b - resp_rx_b) & 0xFFFFFFFFUL);
  double Db = (double)((resp_tx_a - poll_rx_a) & 0xFFFFFFFFUL);

  // printf("🔢 Ra=%.0f, Rb=%.0f, Da=%.0f, Db=%.0f\n", Ra, Rb, Da, Db);

  double tof_dtu = (Ra * Rb - Da * Db) / (Ra + Rb + Da + Db);
  if (tof_dtu < 0)
    tof_dtu = 0;

  double time_s = tof_dtu * DWT_TIME_UNITS;    // thời gian (giây)
  double distance_m = time_s * SPEED_OF_LIGHT; // khoảng cách (m)
  double distance_mm = distance_m * 1000.0;    // khoảng cách (mm)

  // 📝 Gán khoảng cách (mm) vào frame để phản hồi nếu cần
  frame->Data.DIST.Dis = (uint32_t)(distance_mm + 0.5); // Làm tròn
  frame->Cmd = Cmd_Distance; // Đặt lại Cmd để phản hồi khoảng cách

  // printf("✅ Khoang cach toi 0x%06X = %.2f m\n", frame->Src, distance_m);
  //  print_dw_dataframe(frame);

  // ✅ Sau khi đo xong, quay về chế độ định vị TDoA
  // mode_run = mode_tdoa;
}

// Hàm chuyển đổi timestamp sang 64-bit
void tdoa_set_timestamp_u64(uint64_t Ts, uint8_t *Dat) {
  for (int i = 0; i <= 4; i++) // chỉ lấy 5 byte thấp nhất
  {
    Dat[i] = (uint8_t)(Ts & 0xFF);
    Ts >>= 8;
  }
}
// Hàm giải nén timestamp từ 5 byte sang 64-bit
uint64_t tdoa_get_timestamp_u64(const uint8_t *Dat) {
  uint64_t ts = 0;
  for (int i = 4; i >= 0; i--) // giải nén từ 5 byte -> 64-bit
  {
    ts <<= 8;
    ts |= Dat[i];
  }
  return ts;
}

void build_server_frame(const tag_frame_t *tag, uint64_t rx_ts, uint8_t rssi,
                        server_dataframe_t *out) {
  memset(out, 0, sizeof(server_dataframe_t));

  // Gán loại dữ liệu
  out->Type_data = tag->Data.TAG.Type;

  // Gán Packet ID (4 byte)
  memcpy(out->Packit_ID, (const void *)&tag->Packet_Id, 4);

  // Gán Serial_ID = MY_DEVICE_ID (thiết bị hiện tại) - 4 byte
  out->Serial_ID[0] = (uint8_t)(MY_DEVICE_ID & 0xFF);
  out->Serial_ID[1] = (uint8_t)((MY_DEVICE_ID >> 8) & 0xFF);
  out->Serial_ID[2] = (uint8_t)((MY_DEVICE_ID >> 16) & 0xFF);
  out->Serial_ID[3] = (uint8_t)((MY_DEVICE_ID >> 24) & 0xFF);

  // Gán Tag_ID = địa chỉ nguồn của TAG
  memcpy(out->Tag_ID, (const void *)&tag->Src, 4);

  // Gán timestamp chỉ 5 byte thấp
  // memcpy(out->Timestamp, &rx_ts, 5);
  set_timestamp_u64_to_5bytes(rx_ts, out->Timestamp);

  // Gán các trạng thái cảm biến cơ bản
  out->Motion = tag->Data.TAG.Motion;
  out->Button = tag->Data.TAG.Button;
  out->Free_fall =
      tag->Data.TAG.Battery; // dùng battery nếu không có field free-fall riêng
  out->RSSI = rssi;

  // Xử lý tùy theo loại dữ liệu (Cmd)
  switch (tag->Data.TAG.Type) {
  case Cmd_tag_sensor: // ETAG: 3 x 4 byte
    memcpy(out->Type1.Compass, (const void *)&tag->Data.TAG.Custom.ETAG.Compass,
           4);
    memcpy(out->Type1.Pressure,
           (const void *)&tag->Data.TAG.Custom.ETAG.Pressure, 4);
    memcpy(out->Type1.Accelermeter,
           (const void *)&tag->Data.TAG.Custom.ETAG.Acceleration, 4);
    break;

  case Cmd_tag_solut: // SOLUT: 3 x 1 byte
    out->Type2.Temperature = tag->Data.TAG.Custom.SOLUT.Temper;
    out->Type2.Humidity = tag->Data.TAG.Custom.SOLUT.Humi;
    out->Type2.Vibrate = tag->Data.TAG.Custom.SOLUT.Vibra;
    break;

  case Cmd_tag_dps422: // DPS422: Temp(2B), Pressure(4B)
    memcpy(out->Type3.Temperature,
           (const void *)&tag->Data.TAG.Custom.DPS422.Temper, 2);
    memcpy(out->Type3.Pressure,
           (const void *)&tag->Data.TAG.Custom.DPS422.Pressure, 4);
    break;

  case cmd_custom_ducthang:
    memcpy(out->Type5.Version, &tag->Data.TAG.Custom.DUCTHANG.Version, 2);
    memcpy(out->Type5.Temp, &tag->Data.TAG.Custom.DUCTHANG.Temp, 2);
    memcpy(out->Type5.Hi_MAC, &tag->Data.TAG.Custom.DUCTHANG.Hi_MAC, 4);
    memcpy(out->Type5.Low_MAC, &tag->Data.TAG.Custom.DUCTHANG.Low_MAC, 4);
    break;

  case Cmd_tag_nomal: // Xử lý Type 0 giống như Type 5 (Cmd_custom_ducthang)
    memcpy(out->Type0.buff_null, 0, sizeof(out->Type0.buff_null));
    memcpy(out->Type0.buff_null, &tag->Data.TAG.Custom.DUCTHANG.Version, 2);
    memcpy(&out->Type0.buff_null[2], &tag->Data.TAG.Custom.DUCTHANG.Temp, 2);
    memcpy(&out->Type0.buff_null[4], &tag->Data.TAG.Custom.DUCTHANG.Hi_MAC, 4);
    memcpy(&out->Type0.buff_null[8], &tag->Data.TAG.Custom.DUCTHANG.Low_MAC, 4);
    break;

  default: // Loại không xác định
    memset(out->Type0.buff_null, 0, sizeof(out->Type0.buff_null));
    break;
  }

  // Xoá sạch phần Mts_access (không dùng trong TDoA)
  for (int i = 0; i < DECAWAVE_MASTER_ACCESS_NUM; i++) {
    memset(&out->Mts_access[i], 0, sizeof(out->Mts_access[i]));
  }
}

// Hàm in thông tin gói tin DW3000
void print_dw_dataframe(const dw_dataframe_t *frame) {
  printf("\n[DW_FRAME] ==============================\n");
  printf("Des      : %u\n", frame->Des);
  printf("Src      : %u\n", frame->Src);
  printf("PacketId : %u\n", frame->Packet_Id);
  printf("Cmd      : %u\n", frame->Cmd);
  printf("TypeDev  : %u\n", frame->TypeDev);

  switch (frame->Cmd) {
  case Cmd_Sync:
    printf("[SYNC] Ts: ");
    for (int i = 0; i < 5; i++)
      printf("%02X ", frame->Data.SYNC.Ts[i]);
    printf("\n");
    break;

  case Cmd_Distance:
    printf("[DIST] Distance: %u\n", frame->Data.DIST.Dis);
    break;

  case Cmd_Pool:
  case Cmd_Resp:
  case Cmd_Final:
    printf("[DS_TWR]\n");
    printf("  poll_tx : ");
    for (int i = 0; i < 5; i++)
      printf("%02X ", frame->Data.DS_TWR.TIME_STAMP.poll_tx[i]);
    printf("\n  resp_rx : ");
    for (int i = 0; i < 5; i++)
      printf("%02X ", frame->Data.DS_TWR.TIME_STAMP.resp_rx[i]);
    printf("\n  final_tx: ");
    for (int i = 0; i < 5; i++)
      printf("%02X ", frame->Data.DS_TWR.TIME_STAMP.final_tx[i]);
    printf("\n");
    break;

  case Cmd_Tag:
    printf("[TAG] Type=%u, Batt=%u, Motion=%u, Button=%u\n",
           frame->Data.TAG.Type, frame->Data.TAG.Battery,
           frame->Data.TAG.Motion, frame->Data.TAG.Button);

    switch (frame->Data.TAG.Type) {
    case 0x00:
      printf("  [TAG] Type=0 (No payload / default tag)\n");
      break;
    case 0x01: // Cmd_tag_sensor
      printf("  [SENSOR] Compass   : %08X\n",
             frame->Data.TAG.Custom.ETAG.Compass);
      printf("           Pressure  : %08X\n",
             frame->Data.TAG.Custom.ETAG.Pressure);
      printf("           Accel     : %08X\n",
             frame->Data.TAG.Custom.ETAG.Acceleration);
      break;
    case 0x02: // Cmd_tag_solut
      printf("  [SOLUT ] Temp      : %u\n",
             frame->Data.TAG.Custom.SOLUT.Temper);
      printf("           Humi      : %u\n", frame->Data.TAG.Custom.SOLUT.Humi);
      printf("           Vibra     : %u\n", frame->Data.TAG.Custom.SOLUT.Vibra);
      break;
    case 0x03: // Cmd_tag_dps422
      printf("  [DPS422] Temp      : %u\n",
             frame->Data.TAG.Custom.DPS422.Temper);
      printf("           Pressure  : %u\n",
             frame->Data.TAG.Custom.DPS422.Pressure);
      break;
    default:
      printf("  [TAG] Unknown Subtype: %u\n", frame->Data.TAG.Type);
      break;
    }
    break;

  default:
    printf("[UNKNOWN] Khong xac dinh loai goi tin (Cmd=0x%02X)\n", frame->Cmd);
    break;
  }

  printf("CRC      : 0x%04X\n", frame->DCRC);
  printf("=========================================\n");
}

// Hàm in thông tin gói tin server
void print_server_frame(const server_dataframe_t *f) {
  printf("\n[SERVER_FRAME] ==============================\n");
  printf("Type_data : %u\n", f->Type_data);

  uint32_t packet_id = (f->Packit_ID[3] << 24) | (f->Packit_ID[2] << 16) |
                       (f->Packit_ID[1] << 8) | f->Packit_ID[0];
  printf("Packet_ID : %u\n", packet_id);

  uint32_t serial_id = (f->Serial_ID[3] << 24) | (f->Serial_ID[2] << 16) |
                       (f->Serial_ID[1] << 8) | f->Serial_ID[0];
  printf("Serial_ID : %u\n", serial_id);

  uint64_t timestamp = 0;
  for (int i = 4; i >= 0; i--)
    timestamp = (timestamp << 8) | f->Timestamp[i];
  printf("Timestamp : %" PRIu64 "\n", timestamp);

  uint32_t tag_id = (f->Tag_ID[3] << 24) | (f->Tag_ID[2] << 16) |
                    (f->Tag_ID[1] << 8) | f->Tag_ID[0];
  printf("Tag_ID    : %u\n", tag_id);

  printf("Motion    : %u\n", f->Motion);
  printf("Button    : %u\n", f->Button);
  printf("Free_fall : %u\n", f->Free_fall);
  printf("RSSI      : %u\n", f->RSSI);

  switch (f->Type_data) {
  case cmd_custom_ducthang: {
    uint16_t Version; // 2 bytes
    uint16_t Temp;    // 2 bytes
    uint32_t Hi_MAC;  // 4 bytes
    uint32_t Low_MAC; // 4 bytes
    memcpy(&Version, f->Type5.Version, 2);
    memcpy(&Temp, f->Type5.Temp, 2);
    memcpy(&Hi_MAC, f->Type5.Hi_MAC, 4);
    memcpy(&Low_MAC, f->Type5.Low_MAC, 4);
    printf("[DUCTHANG] Version : %u\n", Version);
    printf("           Temp    : %u\n", Temp);
    printf("           MAC     : %02X:%02X:%02X:%02X:%02X\n", Low_MAC & 0xFF,
           (Low_MAC >> 8) & 0xFF, (Low_MAC >> 16) & 0xFF,
           (Low_MAC >> 24) & 0xFF, Hi_MAC & 0xFF);
    break;
  }
  case Cmd_tag_nomal: {
    printf("[NOMAL] No custom data\n");
    break;
  }
  case Cmd_tag_sensor: {
    uint32_t compass = 0, pressure = 0, accel = 0;
    memcpy(&compass, f->Type1.Compass, 4);
    memcpy(&pressure, f->Type1.Pressure, 4);
    memcpy(&accel, f->Type1.Accelermeter, 4);

    printf("[SENSOR] Compass     : %u\n", compass);
    printf("         Pressure    : %u\n", pressure);
    printf("         Accelerome  : %u\n", accel);
    break;
  }

  case Cmd_tag_solut:
    printf("[SOLUT ] Temp        : %u\n", f->Type2.Temperature);
    printf("         Humi        : %u\n", f->Type2.Humidity);
    printf("         Vibra       : %u\n", f->Type2.Vibrate);
    break;

  case Cmd_tag_dps422: {
    uint16_t temp = (f->Type3.Temperature[1] << 8) | f->Type3.Temperature[0];
    uint32_t press = 0;
    memcpy(&press, f->Type3.Pressure, 4);
    printf("[DPS422] Temp        : %u\n", temp);
    printf("         Pressure    : %u\n", press);
    break;
  }

  default:
    printf("[INFO] No extended data.\n");
    break;
  }

  // In thêm raw data nếu cần
  // const uint8_t *raw = (const uint8_t *)f;
  // printf("[RAW DATA HEX]: ");
  // for (int i = 0; i < sizeof(server_dataframe_t); i++)
  //{
  //    printf("%02X ", raw[i]);
  //    if ((i + 1) % 16 == 0)
  //        printf("\n                ");
  //}
  // printf("\n=============================================\n");
}
// Hàm tính RSSI từ dwt_rxdiag_t
int16_t calculate_rssi_dw3000(const dwt_rxdiag_t *diag) {
  if (diag == NULL || diag->ipatovPower == 0 || diag->ipatovAccumCount == 0)
    return -128;

  double ratio = (double)diag->ipatovPower / diag->ipatovAccumCount;
  double rssi_dbm = 10.0 * log10(ratio) - 121.74;

  return (int16_t)(round(rssi_dbm));
}

bool is_duplicate_tag(uint32_t packet_id, uint32_t src) {
  for (int i = 0; i < MAX_TAG_TRACKED; i++) {
    // Nếu packet_id = 0 thì xóa record có src tương ứng
    if (packet_id == 0) {
      if (tag_cache[i].Serial_ID == src) {
        tag_cache[i].Serial_ID = 0;
      }
    }
    // Nếu đã có record và packet_id cũ >= packet_id mới -> trùng
    else if (tag_cache[i].Serial_ID == src &&
             tag_cache[i].Packet_ID >= packet_id) {
      printf("GOI TAG TRUNG: Packet_ID=%lu <= %lu (Src=%lu)\n",
             tag_cache[i].Packet_ID, packet_id, src);
      return true;
    }
  }

  // Lưu packet_id và src vào cache tròn
  tag_cache[tag_cache_index].Serial_ID = src;
  tag_cache[tag_cache_index].Packet_ID = packet_id;
  tag_cache_index = (tag_cache_index + 1) % MAX_TAG_TRACKED;

  return false;
}

// // Hàm gửi gói frame bằng DW3000 (sử dụng delayed TX)
// int dw3000_tx_frame(uint8_t *buffer, uint16_t len)
// {
//     if (len < sizeof(dw_dataframe_t))
//     {
//         printf("❌ dw3000_tx_frame: frame quá nhỏ (%d bytes)\n", len);
//         return DWT_ERROR;
//     }

//     dw_dataframe_t *frame = (dw_dataframe_t *)buffer;

//     // ==== Tính toán thời gian gửi trễ ====
//     uint64_t now = 0;
//     dwt_readsystime((uint8_t *)&now); // Đọc thời gian hiện tại

//     uint64_t tx_timestamp = now + (TX_ANT_DLY * UUS_TO_DWT_TIME) + 100000; //
//     Delay chuẩn uint32_t tx_time = tx_timestamp >> 8; // 32-bit timestamp

//     dwt_setdelayedtrxtime(tx_time); // Cấu hình thời gian gửi trễ

//     // ==== Ghi timestamp TX vào gói OFFSET ====
//     memcpy(frame->Data.SYNC.Ts, &tx_timestamp, 5); // Little endian 40-bit

//     // // ==== Debug thông tin gói sẽ gửi ====
//     // printf("===== 🛰 TX DW FRAME DEBUG =====\n");
//     // printf("Cmd       : 0x%02X\n", frame->Cmd);
//     // printf("Src       : 0x%06lX\n", frame->Src);
//     // printf("Des       : 0x%06lX\n", frame->Des);
//     // printf("Packet ID : %d\n", frame->Packet_Id);
//     // printf("TX TS     : %02X %02X %02X %02X %02X\n",
//     //        frame->Data.SYNC.Ts[0],
//     //        frame->Data.SYNC.Ts[1],
//     //        frame->Data.SYNC.Ts[2],
//     //        frame->Data.SYNC.Ts[3],
//     //        frame->Data.SYNC.Ts[4]);
//     // printf("=================================\n");

//     // ==== Gửi gói tin ====
//     dwt_forcetrxoff();                            // Dừng mọi hoạt động trước
//     dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK); // Clear cờ TX

//     dwt_writetxdata(len, &frame, 0); // Ghi dữ liệu vào FIFO TX
//     dwt_writetxfctrl(len, 0, 1);     // Cấu hình TX (tạm dùng frame standard,
//     ranging = 1)

//     int ret = dwt_starttx(DWT_START_TX_DELAYED); // Bắt đầu gửi ở thời điểm
//     đã định

//     if (ret != DWT_SUCCESS)
//     {
//         printf("❌ dwt_starttx DELAYED thất bại! Có thể quá trễ\n");
//     }
//     // waitforsysstatus(NULL, NULL, DWT_INT_TXFRS_BIT_MASK, 0);
//     // dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);
//     // // ✅ Sau khi gửi xong, bật lại RX
//     // dwt_forcetrxoff();
//     // if (dwt_rxenable(DWT_START_RX_IMMEDIATE) != DWT_SUCCESS)
//     // {
//     //     printf("Khong the bat lai che do RX\n");
//     // }
//     // else
//     // {
//     //     // printf("Da bat lai RX, cho Cmd_Pool\n");
//     // }

//     return ret;
// }

// Hàm gửi gói frame bằng DW3000 (sử dụng delayed TX)
// int dw3000_tx_frame(uint8_t *buffer, uint16_t len)
// {
//     // ==== Chuẩn bị gửi ====
//     dwt_forcetrxoff();                            // Dừng TX/RX nếu đang chạy
//     dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK); // Clear cờ TX hoàn tất

//     dw_dataframe_t *frame = (dw_dataframe_t *)buffer;

//     //== == Debug(nếu cần) == ==
//     // print_dw_offset_frame(frame);

//     // ==== Tính toán thời gian gửi trễ ====
//     // uint64_t now = 0;
//     // dwt_readsystime((uint8_t *)&now); // Đọc thời gian hiện tại (64 bit,
//     little-endian)

//     // uint64_t tx_timestamp = now + (TX_ANT_DLY * UUS_TO_DWT_TIME); // Delay
//     chuẩn
//     // uint32_t tx_time = (uint32_t)(tx_timestamp >> 8);             //
//     40-bit → 32-bit (bỏ 8 bit thấp)

//     // dwt_setdelayedtrxtime(tx_time); // Cấu hình thời gian gửi trễ

//     uint8_t ts[5];
//     dwt_readsystime(ts);               // Đọc 5 byte timestamp từ DW3000
//     uint64_t now = parse_ts_40bit(ts); // Hàm bạn đã viết để ghép 5 byte
//     thành uint64_t

//     uint64_t tx_timestamp = now + (TX_ANT_DLY * UUS_TO_DWT_TIME);
//     uint32_t tx_time = (uint32_t)(tx_timestamp >> 8); // Lấy 40-bit hợp lệ

//     dwt_setdelayedtrxtime(tx_time); // Thiết lập thời điểm TX trễ

//     // == == Ghi timestamp TX vào gói OFFSET == ==
//     // memcpy(frame->Data.SYNC.Ts, &tx_timestamp, 5); // Lưu 5 byte thấp vào
//     frame
//     // for (int i = 0; i < 5; i++)
//     // {
//     //     frame->Data.SYNC.Ts[i] = (tx_timestamp >> (i * 8)) & 0xFF;
//     // }
//     uint64_t tmp_ts = tx_timestamp; // sao chép sang biến tạm

//     for (int i = 0; i < 5; i++)
//     {
//         frame->Data.SYNC.Ts[i] = (tmp_ts >> (i * 8)) & 0xFF;
//     }

//     // Ghi dữ liệu TX và cấu hình frame
//     dwt_writetxdata(len + 2, frame, 0); // buffer là con trỏ uint8_t*, truyền
//     đúng! dwt_writetxfctrl(len + 2, 0, 0);    // ranging = 1

//     int ret = 1;
//     // Gửi bằng DELAYED TX
//     // int ret = dwt_starttx(DWT_START_TX_DELAYED);

//     // if (ret != DWT_SUCCESS)
//     // {
//     //     printf("❌ dwt_starttx DELAYED thất bại! Có thể quá trễ\n");
//     // }
//     if (dwt_starttx(DWT_START_TX_IMMEDIATE) != DWT_SUCCESS)
//     {
//         printf("Gui goi Cmd_start_twr den 0x%06X that bai\n", target_id);
//         return;
//     }
//     ret = DWT_SUCCESS;

//     // waitforsysstatus(NULL, NULL, DWT_INT_TXFRS_BIT_MASK, 0);
//     // dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);

//     // ✅ Sau khi gửi xong, bật lại RX
//     // dwt_forcetrxoff();
//     // if (dwt_rxenable(DWT_START_RX_IMMEDIATE) != DWT_SUCCESS)
//     // {
//     //     printf("Khong the bat lai che do RX\n");
//     // }
//     // else
//     // {
//     //     // printf("Da bat lai RX, cho Cmd_Pool\n");
//     // }

//     return ret;
// }
int dw3000_tx_frame(uint8_t *buffer, uint16_t len) {

  // printf("begin offset \r\n");
  //  ==== Chuẩn bị gửi ====
  dwt_forcetrxoff(); // Dừng TX/RX nếu đang chạy
  dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS_OFFSET);
  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK); // Clear cờ TX hoàn tất

  dw_dataframe_t *frame = (dw_dataframe_t *)buffer;

  // ==== Ghi dữ liệu TX ====
  dwt_writetxdata(len + 2, (uint8_t *)frame, 0); // Ghi data
  dwt_writetxfctrl(len + 2, 0, 0);               // Frame ctrl (ranging = 1)

  // ==== Gửi ngay lập tức ====
  if (dwt_starttx(DWT_START_TX_IMMEDIATE) != DWT_SUCCESS) {
    // printf("❌ Gửi gói tin thất bại (starttx immediate)\n");
    return DWT_ERROR;
  }

  // ==== Chờ TX hoàn tất ====
  waitforsysstatus(NULL, NULL, DWT_INT_TXFRS_BIT_MASK, 0);
  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK);

  // ==== Đọc lại TX timestamp thật sự ====
  uint8_t tx_ts_buf[5];
  dwt_readtxtimestamp(tx_ts_buf);             // Đọc 5 byte timestamp
  uint64_t tx_ts = parse_ts_40bit(tx_ts_buf); // Chuyển thành uint64_t

  // ==== Ghi timestamp vào frame ====
  for (int i = 0; i < 5; i++) {
    frame->Data.SYNC.Ts[i] = (tx_ts >> (i * 8)) & 0xFF;
  }
  // printf("end offset \r\n");
  //  ✅ Gửi thành công
  return DWT_SUCCESS;
}

void print_dw_offset_frame(const dw_dataframe_t *frame) {
  // if (frame == NULL)
  //{
  //     printf("[ERR] frame NULL\n");
  //     return;
  // }

  // printf("===== 📥 OFFSET FRAME RX DEBUG =====\n");
  // printf("Cmd        : 0x%02X\n", frame->Cmd);
  // printf("Src        : 0x%06lX\n", frame->Src);
  // printf("Des        : 0x%06lX\n", frame->Des);
  // printf("Packet_ID  : %d\n", frame->Packet_Id);

  //// In timestamp dạng hex
  // printf("Timestamp  : %02X %02X %02X %02X %02X\n",
  //        frame->Data.SYNC.Ts[0],
  //        frame->Data.SYNC.Ts[1],
  //        frame->Data.SYNC.Ts[2],
  //        frame->Data.SYNC.Ts[3],
  //        frame->Data.SYNC.Ts[4]);

  //// Ép thành uint64_t để in giá trị đầy đủ
  // uint64_t ts = 0;
  // memcpy(&ts, frame->Data.SYNC.Ts, 5); // 5 byte little endian
  // printf("Timestamp (uint64_t): %llu\n", ts);

  // printf("====================================\n");
}

void set_timestamp_u64_to_5bytes(uint64_t timestamp, uint8_t ts[5]) {
  for (int i = 0; i < 5; i++) {
    ts[i] = (timestamp >> (8 * i)) & 0xFF;
  }
}

uint64_t parse_ts_40bit(const uint8_t ts[5]) {
  uint64_t result = 0;
  for (int i = 4; i >= 0; i--) {
    result = (result << 8) | ts[i];
  }
  return result;
}
