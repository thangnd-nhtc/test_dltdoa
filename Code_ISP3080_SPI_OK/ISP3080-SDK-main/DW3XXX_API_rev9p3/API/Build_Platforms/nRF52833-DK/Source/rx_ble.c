#include "rx_ble.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_gap.h"
#include "ble_hci.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_log.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "sdk_errors.h"
#include "tx_ble.h"
#include "Source/broadcast_twr.h"

#define APP_COMPANY_IDENTIFIER 0x0059 // Nordic ID
#define APP_DEVICE_TYPE 0x02          // iBeacon device type
#define APP_ADV_DATA_LENGTH 0x15      // 21 bytes payload

// ==== Scan parameters ====
static ble_gap_scan_params_t m_scan_params = {
    .active = 0,
    .interval = 160,   // 100 ms (unit 0.625ms)
    .window = 160,     // 100 ms -> Quét liên tục 100%, không bỏ lỡ ACK
    .timeout = 0x0000, // No timeout
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,
    .scan_phys = BLE_GAP_PHY_1MBPS,
};

static uint8_t m_scan_buffer_data[BLE_GAP_SCAN_BUFFER_MIN];
static ble_data_t m_scan_buffer = {.p_data = m_scan_buffer_data,
                                   .len = BLE_GAP_SCAN_BUFFER_MIN};

static bool m_is_scanning = false;

void rx_ble_init(void) {
  // Initialization flag
  m_is_scanning = false;
}

void rx_ble_scan_start(void) {
  ret_code_t err_code;
  if (m_is_scanning)
    return;

  err_code = sd_ble_gap_scan_start(&m_scan_params, &m_scan_buffer);
  if (err_code == NRF_SUCCESS) {
    m_is_scanning = true;
  } else {
    printf("[RX_BLE] Scan Start Error: 0x%08x\n", (unsigned int)err_code);
  }
}

void rx_ble_scan_stop(void) {
  if (!m_is_scanning)
    return;

  sd_ble_gap_scan_stop();
  m_is_scanning = false;
}

/**
 * @brief Analysis of advertising data for iBeacon/Config ACK
 */
static void parse_adv_report(ble_gap_evt_adv_report_t const *p_adv_report) {
  uint8_t *p_data = (uint8_t *)p_adv_report->data.p_data;
  uint16_t data_len = p_adv_report->data.len;
  uint16_t offset = 0;


  while (offset < data_len) {
    uint8_t len = p_data[offset];
    if (len == 0)
      break;

    uint8_t type = p_data[offset + 1];

    if (type == 0xFF) { // Manufacturer Specific Data
      uint16_t company_id = p_data[offset + 2] | (p_data[offset + 3] << 8);
      if (company_id == APP_COMPANY_IDENTIFIER) {
        // Chỉ xử lý nếu đúng cấu trúc của chúng ta
        if (p_data[offset + 4] == APP_DEVICE_TYPE) {
          uint8_t *payload = &p_data[offset + 4];

          // === Broadcast TWR: Thu thập Tag ID từ ACK ===
          if (bcast_twr_is_enabled()) {
            extern uint8_t my_base_id_raw[5];
            // Kiểm tra Base ID ở byte 13-17 có phải mình không
            if (memcmp(&payload[13], my_base_id_raw, 5) == 0) {
              // Tag ID nằm ở byte 8-12
              bcast_twr_on_ack_received(&payload[8], p_adv_report->rssi);
            }
          }

          // Chuyển payload qua tx_ble để check ACK (Unicast)
          tx_ble_on_ack_received(payload);

          // Chuyển payload qua check Request Fragment từ Tag
          tx_ble_on_tag_fragment_received(payload);
        }
      }
    }
    offset += (len + 1);
  }
}

/**
 * @brief BLE observer callback
 */
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context) {
  ret_code_t err_code;
  switch (p_ble_evt->header.evt_id) {
  case BLE_GAP_EVT_ADV_REPORT:
    parse_adv_report(&p_ble_evt->evt.gap_evt.params.adv_report);

    // Re-start scanning to receive next packets (required by S140)
    if (m_is_scanning) {
      err_code = sd_ble_gap_scan_start(NULL, &m_scan_buffer);
      if (err_code != NRF_SUCCESS && err_code != NRF_ERROR_INVALID_STATE) {
        // printf("[RX_BLE] Scan restart failed: 0x%x\n", err_code);
      }
    }
    break;

  case BLE_GAP_EVT_TIMEOUT:
    if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN) {
      m_is_scanning = false;
      printf("[RX_BLE] Scan Timeout -> Restarting...\n");
      rx_ble_scan_start();
    }
    break;

  default:
    break;
  }
}

// Register as a BLE observer
NRF_SDH_BLE_OBSERVER(m_rx_ble_observer, 3, ble_evt_handler, NULL);
