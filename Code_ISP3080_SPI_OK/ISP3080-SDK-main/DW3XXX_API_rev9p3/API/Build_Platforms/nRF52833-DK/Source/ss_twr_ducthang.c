/* SS-TWR Initiator (Node A) - Exactly matching TRILE pattern */
#include "ss_twr_ducthang.h"
#include "DW3000.h"
#include "deca_device_api.h"
#include "main.h"
#include "port.h"
#include "shared_defines.h"
#include "shared_functions.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Timing - Rêng biệt cho DucThang để tránh bị DW3000.h ghi đè
 * (RESP_RX_TIMEOUT_UUS) */
#define DUCTHANG_POLL_RX_DLY_UUS 240
// #define DUCTHANG_POLL_RX_DLY_UUS 3000
//#define DUCTHANG_RX_TIMEOUT_UUS 294000
// #define DUCTHANG_RX_TIMEOUT_UUS 70000
#define DUCTHANG_RX_TIMEOUT_UUS 5000

static uint8_t tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V',
                                'E', 0xE0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0};
static uint8_t rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W',
                                'A', 0xE1, 0, 0, 0, 0, 0, 0,
                                0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF, 0, 0};

static uint8_t frame_seq_nb = 0;
static uint8_t twr_rx_buffer[64];
static uint32_t twr_status_reg = 0;
static float twr_buff[TWR_BUFFER_LENGHT];
static uint8_t index_twr = 0;
static double distance = 0;
static bool config_applied = false;

extern uint8_t my_base_id_raw[5];
extern uint8_t current_tag_id_raw[5];
static char uart_tx_buf[128];

static float median_filter(float *data, int size)
{
  float temp[size];
  memcpy(temp, data, sizeof(float) * size);
  for (int i = 0; i < size - 1; i++)
    for (int j = i + 1; j < size; j++)
      if (temp[i] > temp[j])
      {
        float t = temp[i];
        temp[i] = temp[j];
        temp[j] = t;
      }
  return temp[size / 2];
}

bool ss_twr_ducthang_handle(void)
{
  if (!config_applied)
  {
    dwt_configure(&config);
    dwt_settxantennadelay(TX_ANT_DLY);
    dwt_setrxantennadelay(RX_ANT_DLY);
    config_applied = true;
  }

  /* 1. convert chip to idle */
  dwt_forcetrxoff();

  /* 2. Cấu hình LẠI timing cho TWR (Vô cùng quạn trọng vì poll_rx_once set
   * timeout = 0) */
  dwt_setrxaftertxdelay(DUCTHANG_POLL_RX_DLY_UUS);
  dwt_setrxtimeout(DUCTHANG_RX_TIMEOUT_UUS);

  /* 3. Gán ID và Sequence Number */
  memcpy(&tx_poll_msg[10], my_base_id_raw, 5);
  memcpy(&tx_poll_msg[15], current_tag_id_raw, 5);
  tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;

  /* 4. Phát Poll */
  dwt_writesysstatuslo(DWT_INT_TXFRS_BIT_MASK | SYS_STATUS_ALL_RX_TO |
                       SYS_STATUS_ALL_RX_ERR);
  dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0);
  dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1);
  int ret = dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

  // printf(">>> POLL TX [%d]: ", frame_seq_nb);
  // for (int i = 0; i < (int)sizeof(tx_poll_msg); i++)
  //   printf("%02X ", tx_poll_msg[i]);
  // printf("\n");
  // printf(">>> Expected RX Timeout (dwt_setrxtimeout): %d UUS\n",
  //        DUCTHANG_RX_TIMEOUT_UUS);

  if (ret != DWT_SUCCESS)
  {
    printf(">>> TWR TX FAILED (ret=%d)! Chip IDLE.\n", ret);
    dwt_forcetrxoff(); // Reset chip state
    // Sleep(RNG_DELAY_MS);
    return false; // Do not enter waitforsysstatus, it will hang!
  }

  /* We assume that the transmission is achieved correctly, poll for reception
   * of a frame or error/timeout. See below for the addition of a timeout
   * to prevent infinite hang. */
  {
    uint32_t lo_mask =
        DWT_INT_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR;
    uint32_t timeout_cnt = 0;
    twr_status_reg = 0;
    while (!(twr_status_reg & lo_mask))
    {
      twr_status_reg = dwt_readsysstatuslo();
      if (++timeout_cnt > 500000)
      { // ~500ms trên nRF52833 @64MHz
        // printf(">>> TWR: waitforsysstatus TIMEOUT! Force IDLE.\n");
        dwt_forcetrxoff();
        dwt_writesysstatuslo(SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        distance = -1.0;
        Sleep(RNG_DELAY_MS);
        return false;
      }
    }
  }

  /* 5. Tăng Sequence Number ngay */
  frame_seq_nb++;

  if (twr_status_reg & DWT_INT_RXFCG_BIT_MASK)
  {
    /* === THÀNH CÔNG === */
    dwt_writesysstatuslo(DWT_INT_RXFCG_BIT_MASK);
    uint16_t frame_len = dwt_getframelength(0);
    if (frame_len <= sizeof(twr_rx_buffer))
    {
      dwt_readrxdata(twr_rx_buffer, frame_len, 0);
      twr_rx_buffer[ALL_MSG_SN_IDX] = 0;

      if (memcmp(twr_rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0)
      {
        if (memcmp(&twr_rx_buffer[18], current_tag_id_raw, 5) == 0 &&
            memcmp(&twr_rx_buffer[23], my_base_id_raw, 5) == 0)
        {

          uint32_t poll_tx_ts, resp_rx_ts, poll_rx_ts, resp_tx_ts;
          float clockOffsetRatio;

          poll_tx_ts = dwt_readtxtimestamplo32();
          resp_rx_ts = dwt_readrxtimestamplo32(0);
          clockOffsetRatio =
              ((float)dwt_readclockoffset()) / (uint32_t)(1 << 26);
          resp_msg_get_ts(&twr_rx_buffer[RESP_MSG_POLL_RX_TS_IDX], &poll_rx_ts);
          resp_msg_get_ts(&twr_rx_buffer[RESP_MSG_RESP_TX_TS_IDX], &resp_tx_ts);

          double tof =
              (((double)(resp_rx_ts - poll_tx_ts) -
                (double)(resp_tx_ts - poll_rx_ts) * (1.0 - clockOffsetRatio)) /
               2.0) *
              DWT_TIME_UNITS;
          // float dist_raw = (float)(tof * SPEED_OF_LIGHT - CALIB_TWR);
          float dist_raw = (float)(tof * SPEED_OF_LIGHT);

          // if (index_twr < TWR_BUFFER_LENGHT) {
          //   twr_buff[index_twr++] = dist_raw;
          // } else {
          //   index_twr = 0;
          //   distance = distance * 0.7 +
          //              median_filter(twr_buff, TWR_BUFFER_LENGHT) * 0.3;
          distance = dist_raw;
          // if (distance < 0)
          //   distance = 0;
          // sprintf(uart_tx_buf, "{\"DIST\":%3.2f}\n", (float)distance);
          // printf("[DucThang] %s", uart_tx_buf);
          //}
          /* Thành công: Không Sleep cứng nữa để tối đa tốc độ quét */
          // Cần nghỉ xíu chừng 100us để DWM có thời gian xả trạng thái
          nrf_delay_us(50);
          return true;
        }
      }
    }
    dwt_writesysstatuslo(SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
  }
  else
  {
    // printf(">>> TWR: RX TIMEOUT\n");
    dwt_writesysstatuslo(SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
  }
  /* Không đo được -> gán distance = -1 để U7 biết "không tìm thấy" */
  distance = -1.0;
  nrf_delay_us(50);
  return false;
}

float ss_twr_ducthang_get_distance(void) { return (float)distance; }

void ss_twr_ducthang_cleanup(void)
{
  config_applied = false; // Reset để lần sau vào SS-TWR sẽ apply lại config TWR
}
