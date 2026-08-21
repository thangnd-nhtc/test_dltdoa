#include "request.h"
#include "Source/ducthang_flash.h" // For g_beacon_cfg
#include "main.h"
#include "tx_ble.h"
#include <stdio.h>
#include <string.h>

request_data_t g_request_data;

void request_init(void) {
  memset(&g_request_data, 0, sizeof(request_data_t));
  // --- State mặc định ---
  g_request_data.current_state = TAG_STATE_DEFAULT;
}

// ============================================================
//  request_print_data – In dữ liệu ra Console để Debug
// ============================================================
void request_print_data(uint8_t request_type) {
  if (request_type == 1) { // TIMER
    printf("------- TAG TIMER DATA -------\n");
    printf("Motion: %d \n", g_request_data.timer.send_tag_motion);
    printf("Stand: %d \n", g_request_data.timer.send_tag_stand);
    printf("Sleep1: %d \n", g_request_data.timer.send_tag_sleep1);
    printf("Sleep2: %d \n", g_request_data.timer.send_tag_sleep2);
    printf("Sleep3: %d \n", g_request_data.timer.send_tag_sleep3);
    printf("Interval Sleep1: %d \n", g_request_data.timer.sleep_mode1);
    printf("Interval Sleep2: %d \n", g_request_data.timer.sleep_mode2);
    printf("Interval Sleep3: %d \n", g_request_data.timer.sleep_mode3);
    printf("Batt Default: %d \n", g_request_data.timer.batt_default);
    printf("Batt High: %d \n", g_request_data.timer.batt_high);
    printf("Batt Increase: %d\n", g_request_data.timer.batt_increase);
    printf("Batt Decrease: %d\n", g_request_data.timer.batt_decrease);
    printf("------------------------------\n");
  } else if (request_type == 2) { // CONFIG
    printf("------- TAG CONFIG DATA ------\n");
    printf("Chan: %d, Plen: %d, PAC: %d\n", g_request_data.config.uwb_chan,
           g_request_data.config.uwb_plen, g_request_data.config.uwb_pac);
    printf("TX Code: %d, RX Code: %d, SFD Type: %d\n",
           g_request_data.config.uwb_txcode, g_request_data.config.uwb_rxcode,
           g_request_data.config.uwb_sfdtype);
    printf("DataRate: %d, PHR Mode: %d, PHR Rate: %d\n",
           g_request_data.config.uwb_datarate,
           g_request_data.config.uwb_phrmode,
           g_request_data.config.uwb_phrrate);
    printf("SFD TO: %d, STS Mode: %d, STS Len: %d, PDOA: %d\n",
           g_request_data.config.uwb_sfdto, g_request_data.config.uwb_stsmode,
           g_request_data.config.uwb_stslen, g_request_data.config.uwb_pdoa);
    printf("------------------------------\n");
  } else if (request_type == 3) { // STATE
    printf("------- TAG STATE DATA -------\n");
    printf("Current State: %d\n", g_request_data.current_state);
    printf("------------------------------\n");
  } else if (request_type == 4) { // ID TAG
    printf("------- TAG ID DATA ----------\n");
    printf("ID: %02X %02X %02X %02X %02X\n",
           g_request_data.id_tag.id[0], g_request_data.id_tag.id[1],
           g_request_data.id_tag.id[2], g_request_data.id_tag.id[3],
           g_request_data.id_tag.id[4]);
    printf("------------------------------\n");
  }
}

// ============================================================
//  request_on_tag_fragment_received – Base nhận Fragment từ Tag
// ============================================================
void request_on_tag_fragment_received(uint16_t major, uint8_t minor) {
  switch (major) {
  case MAJOR_TIMER_SEND_TAG_MOTION:
    g_request_data.timer.send_tag_motion = minor;
    break;
  case MAJOR_TIMER_SEND_TAG_STAND:
    g_request_data.timer.send_tag_stand = minor;
    break;
  case MAJOR_TIMER_SEND_TAG_SLEEP_MODE_1:
    g_request_data.timer.send_tag_sleep1 = minor;
    break;
  case MAJOR_TIMER_SEND_TAG_SLEEP_MODE_2:
    g_request_data.timer.send_tag_sleep2 = minor;
    break;
  case MAJOR_TIMER_SEND_TAG_SLEEP_MODE_3:
    g_request_data.timer.send_tag_sleep3 = minor;
    break;
  case MAJOR_TIMER_SLEEP_MODE_1:
    g_request_data.timer.sleep_mode1 = minor;
    break;
  case MAJOR_TIMER_SLEEP_MODE_2:
    g_request_data.timer.sleep_mode2 = minor;
    break;
  case MAJOR_TIMER_SLEEP_MODE_3:
    g_request_data.timer.sleep_mode3 = minor;
    break;
  case MAJOR_BATT_UPDATE_DEFAULT:
    g_request_data.timer.batt_default = minor;
    break;
  case MAJOR_BATT_UPDATE_HIGH:
    g_request_data.timer.batt_high = minor;
    break;
  case MAJOR_BATT_INCREASE:
    g_request_data.timer.batt_increase = minor;
    break;
  case MAJOR_BATT_DECREASE:
    g_request_data.timer.batt_decrease = minor;
    break;

  case MAJOR_CONFIG_UWB_CHAN:
    g_request_data.config.uwb_chan = minor;
    break;
  case MAJOR_CONFIG_UWB_PLEN:
    g_request_data.config.uwb_plen = minor;
    break;
  case MAJOR_CONFIG_UWB_PAC:
    g_request_data.config.uwb_pac = minor;
    break;
  case MAJOR_CONFIG_UWB_TXCODE:
    g_request_data.config.uwb_txcode = minor;
    break;
  case MAJOR_CONFIG_UWB_RXCODE:
    g_request_data.config.uwb_rxcode = minor;
    break;
  case MAJOR_CONFIG_UWB_SFDTYPE:
    g_request_data.config.uwb_sfdtype = minor;
    break;
  case MAJOR_CONFIG_UWB_DATARATE:
    g_request_data.config.uwb_datarate = minor;
    break;
  case MAJOR_CONFIG_UWB_PHRMODE:
    g_request_data.config.uwb_phrmode = minor;
    break;
  case MAJOR_CONFIG_UWB_PHRRATE:
    g_request_data.config.uwb_phrrate = minor;
    break;
  case MAJOR_CONFIG_UWB_SFDTO:
    if ((minor & 0x80) == 0) {
      // Fragment A: bit7 = 0 -> 7 bit cao
      g_request_data.config.uwb_sfdto =
          (g_request_data.config.uwb_sfdto & 0x007F) | ((minor & 0x7F) << 7);
    } else {
      // Fragment B: bit7 = 1 -> 7 bit thấp
      g_request_data.config.uwb_sfdto =
          (g_request_data.config.uwb_sfdto & 0xFF80) | (minor & 0x7F);
    }
    break;
  case MAJOR_CONFIG_UWB_STSMODE:
    g_request_data.config.uwb_stsmode = minor;
    break;
  case MAJOR_CONFIG_UWB_STSLEN:
    g_request_data.config.uwb_stslen = minor;
    break;
  case MAJOR_CONFIG_UWB_PDOA:
    g_request_data.config.uwb_pdoa = minor;
    break;

  case MAJOR_STATE_DEFAULT:
    g_request_data.current_state = TAG_STATE_DEFAULT;
    break;
  case MAJOR_STATE_TX:
    g_request_data.current_state = TAG_STATE_TX;
    break;
  case MAJOR_STATE_OFF_UWB:
    g_request_data.current_state = TAG_STATE_OFF_UWB;
    break;
  case MAJOR_STATE_SOS:
    g_request_data.current_state = TAG_STATE_SOS;
    break;
  case MAJOR_STATE_IDENTIFY:
    g_request_data.current_state = TAG_STATE_IDENTIFY;
    break;
  case MAJOR_STATE_TWR:
    g_request_data.current_state = TAG_STATE_TWR;
    break;
  case MAJOR_STATE_RESET:
    g_request_data.current_state = TAG_STATE_RESET;
    break;
  case MAJOR_STATE_AIRPLAN:
    g_request_data.current_state = TAG_STATE_AIRPLAN;
    break;
  case MAJOR_STATE_MOTION:
    g_request_data.current_state = TAG_STATE_MOTION;
    break;

  case MAJOR_ID_CHANGE_BYTE1:
    g_request_data.id_tag.id[0] = minor;
    break;
  case MAJOR_ID_CHANGE_BYTE2:
    g_request_data.id_tag.id[1] = minor;
    break;
  case MAJOR_ID_CHANGE_BYTE3:
    g_request_data.id_tag.id[2] = minor;
    break;
  case MAJOR_ID_CHANGE_BYTE4:
    g_request_data.id_tag.id[3] = minor;
    break;
  case MAJOR_ID_CHANGE_BYTE5:
    g_request_data.id_tag.id[4] = minor;
    break;
  case MAJOR_REQUEST_TIMER:
    memset(&g_request_data, 0, sizeof(g_request_data));
    // printf("[BLE] >>> Start receiving [TIMER] Struct from Tag...\n");
    break;
  case MAJOR_REQUEST_CONFIG:
    memset(&g_request_data, 0, sizeof(g_request_data));
    // printf("[BLE] >>> Start receiving [CONFIG] Struct from Tag...\n");
    break;
  case MAJOR_REQUEST_STATE:
    memset(&g_request_data, 0, sizeof(g_request_data));
    // printf("[BLE] >>> Start receiving [STATE] Struct from Tag...\n");
    break;
  case MAJOR_REQUEST_ID_TAG:
    memset(&g_request_data, 0, sizeof(g_request_data));
    // printf("[BLE] >>> Start receiving [ID TAG] Struct from Tag...\n");
    break;
  }

  // ============================================================
  // Kiểm tra cờ Finish dựa trên biến g_beacon_cfg.val_request
  // (Cho biết Base đang Request trường gì)
  // ============================================================
  bool is_finished = false;

  if (g_beacon_cfg.val_request == 1) { // 1 = TIMER
    if (major == MAJOR_BATT_DECREASE) {
      is_finished = true;
      // printf(
      //     "[BLE] >>> [REQUEST MODE] Đã nhận ĐỦ toàn bộ STRUCT TIMER từ
      //     Tag!\n");
    }
  } else if (g_beacon_cfg.val_request == 2) { // 2 = CONFIG
    if (major == MAJOR_CONFIG_UWB_PDOA) {
      is_finished = true;
      // printf("[BLE] >>> [REQUEST MODE] Đã nhận ĐỦ toàn bộ STRUCT CONFIG từ "
      //        "Tag!\n");
    }
  } else if (g_beacon_cfg.val_request == 3) { // 3 = STATE
    // State chỉ có 1 gói nên nhận cái là xong luôn
    if ((major >> 8) == 0x20) { // All State Majors are 0x20xx
      is_finished = true;
      // printf(
      //     "[BLE] >>> [REQUEST MODE] Đã nhận ĐỦ (1 gói) STRUCT STATE từ
      //     Tag!\n");
    }
  } else if (g_beacon_cfg.val_request == 4) { // 4 = ID TAG
    if (major == MAJOR_ID_CHANGE_BYTE5) {
      is_finished = true;
    }
  }
  // liệu ở biến "g_request_data"
  if (is_finished) {
    // request_print_data(g_beacon_cfg.val_request);
    g_beacon_cfg.val_request = 0;

    // Kích hoạt Trigger báo về U7 qua IRQ SPI
    spi_send_request_done();
  }
}
