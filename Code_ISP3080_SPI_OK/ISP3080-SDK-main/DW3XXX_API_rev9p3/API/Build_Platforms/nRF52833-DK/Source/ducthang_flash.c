#include "ducthang_flash.h"
#include "nrf_sdh_soc.h"
#include "nrf_soc.h"
#include <string.h>

beacon_cfg_t g_beacon_cfg;
beacon_cfg_t g_pending_beacon_cfg;     // Lưu dể dùng sau khi memset 0
bool flag_save_config_pending = false; // Cờ chờ lưu flash

// Mapping options to DW3000 API values
static const uint8_t chan_map[] = {0, 5, 9};
static const uint16_t plen_map[] = {0,
                                    DWT_PLEN_4096,
                                    DWT_PLEN_2048,
                                    DWT_PLEN_1536,
                                    DWT_PLEN_1024,
                                    DWT_PLEN_512,
                                    DWT_PLEN_256,
                                    DWT_PLEN_128,
                                    DWT_PLEN_72,
                                    DWT_PLEN_64,
                                    DWT_PLEN_32};
static const dwt_pac_size_e pac_map[] = {0, DWT_PAC8, DWT_PAC16, DWT_PAC32,
                                         DWT_PAC4};
static const uint8_t stsmode_map[] = {0,
                                      DWT_STS_MODE_OFF,
                                      DWT_STS_MODE_1,
                                      DWT_STS_MODE_2,
                                      DWT_STS_MODE_ND,
                                      DWT_STS_MODE_SDC,
                                      DWT_STS_CONFIG_MASK};
static const uint8_t stslen_map[] = {0,
                                     DWT_STS_LEN_32,
                                     DWT_STS_LEN_64,
                                     DWT_STS_LEN_128,
                                     DWT_STS_LEN_256,
                                     DWT_STS_LEN_512,
                                     DWT_STS_LEN_1024,
                                     DWT_STS_LEN_2048};
static const uint8_t pdoa_map[] = {0, DWT_PDOA_M0, DWT_PDOA_M1, DWT_PDOA_M3};

static volatile bool m_flash_busy = false;

static void soc_evt_handler(uint32_t evt_id, void *p_context) {
  if (evt_id == NRF_EVT_FLASH_OPERATION_SUCCESS ||
      evt_id == NRF_EVT_FLASH_OPERATION_ERROR) {
    m_flash_busy = false;
  }
}
NRF_SDH_SOC_OBSERVER(m_soc_observer, 0, soc_evt_handler, NULL);

void load_dwt_config(void) {
  flash_dwt_data_t *p_data = (flash_dwt_data_t *)FLASH_START_ADDR;
  if (p_data->magic == DWT_CONFIG_MAGIC) {
    // Sanity check: Channel must be 5 or 9
    if (p_data->cfg.chan != 5 && p_data->cfg.chan != 9) {
      printf("[Flash] Invalid channel %d in flash, using defaults\n",
             p_data->cfg.chan);
      // Do not update config, it stays at default
    } else {
      memcpy(&config, &p_data->cfg, sizeof(dwt_config_t));
      g_beacon_cfg.uwb_chan = p_data->uwb_options[0];
      g_beacon_cfg.uwb_plen = p_data->uwb_options[1];
      g_beacon_cfg.uwb_pac = p_data->uwb_options[2];
      g_beacon_cfg.uwb_txcode = p_data->uwb_options[3];
      g_beacon_cfg.uwb_rxcode = p_data->uwb_options[4];
      g_beacon_cfg.uwb_sfdtype = p_data->uwb_options[5];
      g_beacon_cfg.uwb_datarate = p_data->uwb_options[6];
      g_beacon_cfg.uwb_phrmode = p_data->uwb_options[7];
      g_beacon_cfg.uwb_phrrate = p_data->uwb_options[8];
      g_beacon_cfg.uwb_sfdto = p_data->uwb_options[9];
      g_beacon_cfg.uwb_stsmode = p_data->uwb_options[10];
      g_beacon_cfg.uwb_stslen = p_data->uwb_options[11];
      g_beacon_cfg.uwb_pdoa = p_data->uwb_options[12];
      // printf("[Flash] Loaded config and options from flash (CH:%d)\n",
      //        config.chan);
      // printf("   -> txPreambLength: 0x%02X\n", config.txPreambLength);
      // printf("   -> rxPAC: 0x%02X\n", config.rxPAC);
      // printf("   -> txCode: %d, rxCode: %d\n", config.txCode, config.rxCode);
      // printf("   -> sfdType: %d\n", config.sfdType);
      // printf("   -> dataRate: %d\n", config.dataRate);
      // printf("   -> phrMode: %d, phrRate: %d\n", config.phrMode,
      //        config.phrRate);
      // printf("   -> sfdTO: %d\n", config.sfdTO);
      // printf("   -> stsMode: %d, stsLength: 0x%02X\n", config.stsMode,
      //        config.stsLength);
      // printf("   -> pdoaMode: %d\n", config.pdoaMode);
    }
  } else {
    // Save default config
    uint32_t
        __attribute__((aligned(4))) buffer[(sizeof(flash_dwt_data_t) + 3) / 4];
    memset(buffer, 0, sizeof(buffer));
    flash_dwt_data_t *p_out = (flash_dwt_data_t *)buffer;
    p_out->magic = DWT_CONFIG_MAGIC;
    memcpy(&p_out->cfg, &config, sizeof(dwt_config_t));
    memset(p_out->uwb_options, 0, 13);

    ret_code_t err_code;
    m_flash_busy = true;
    err_code = sd_flash_page_erase(FLASH_PAGE_NUM);
    if (err_code == NRF_SUCCESS) {
      while (m_flash_busy) {
      } // Wait for event
    } else {
      m_flash_busy = false;
      printf("[Flash] Erase failed error: %lu\n", err_code);
    }

    m_flash_busy = true;
    err_code = sd_flash_write((uint32_t *)FLASH_START_ADDR, buffer,
                              sizeof(buffer) / 4);
    if (err_code == NRF_SUCCESS) {
      while (m_flash_busy) {
      } // Wait for event
    } else {
      m_flash_busy = false;
      printf("[Flash] Write failed error: %lu\n", err_code);
    }
    printf("[Flash] Saved default config to flash\n");
  }
}

void cache_pending_dwt_config(void) {
  memcpy(&g_pending_beacon_cfg, &g_beacon_cfg, sizeof(beacon_cfg_t));
}

void apply_and_save_dwt_config(void) {
  bool changed = false;
  if (g_pending_beacon_cfg.uwb_chan > 0 && g_pending_beacon_cfg.uwb_chan <= 2) {
    config.chan = chan_map[g_pending_beacon_cfg.uwb_chan];
    g_beacon_cfg.uwb_chan = g_pending_beacon_cfg.uwb_chan;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_plen > 0 &&
      g_pending_beacon_cfg.uwb_plen <= 10) {
    config.txPreambLength = plen_map[g_pending_beacon_cfg.uwb_plen];
    g_beacon_cfg.uwb_plen = g_pending_beacon_cfg.uwb_plen;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_pac > 0 && g_pending_beacon_cfg.uwb_pac <= 4) {
    config.rxPAC = pac_map[g_pending_beacon_cfg.uwb_pac];
    g_beacon_cfg.uwb_pac = g_pending_beacon_cfg.uwb_pac;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_txcode > 0) {
    config.txCode = g_pending_beacon_cfg.uwb_txcode;
    g_beacon_cfg.uwb_txcode = g_pending_beacon_cfg.uwb_txcode;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_rxcode > 0) {
    config.rxCode = g_pending_beacon_cfg.uwb_rxcode;
    g_beacon_cfg.uwb_rxcode = g_pending_beacon_cfg.uwb_rxcode;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_sfdtype > 0) {
    config.sfdType = g_pending_beacon_cfg.uwb_sfdtype; // 1, 2, 3 mapped as-is
    g_beacon_cfg.uwb_sfdtype = g_pending_beacon_cfg.uwb_sfdtype;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_datarate > 0 &&
      g_pending_beacon_cfg.uwb_datarate <= 3) {
    config.dataRate = g_pending_beacon_cfg.uwb_datarate - 1;
    g_beacon_cfg.uwb_datarate = g_pending_beacon_cfg.uwb_datarate;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_phrmode > 0 &&
      g_pending_beacon_cfg.uwb_phrmode <= 2) {
    config.phrMode = g_pending_beacon_cfg.uwb_phrmode - 1;
    g_beacon_cfg.uwb_phrmode = g_pending_beacon_cfg.uwb_phrmode;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_phrrate > 0 &&
      g_pending_beacon_cfg.uwb_phrrate <= 2) {
    config.phrRate = g_pending_beacon_cfg.uwb_phrrate - 1;
    g_beacon_cfg.uwb_phrrate = g_pending_beacon_cfg.uwb_phrrate;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_sfdto > 0) {
    config.sfdTO = g_pending_beacon_cfg.uwb_sfdto;
    g_beacon_cfg.uwb_sfdto = g_pending_beacon_cfg.uwb_sfdto;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_stsmode > 0 &&
      g_pending_beacon_cfg.uwb_stsmode <= 6) {
    config.stsMode = stsmode_map[g_pending_beacon_cfg.uwb_stsmode];
    g_beacon_cfg.uwb_stsmode = g_pending_beacon_cfg.uwb_stsmode;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_stslen > 0 &&
      g_pending_beacon_cfg.uwb_stslen <= 7) {
    config.stsLength = stslen_map[g_pending_beacon_cfg.uwb_stslen];
    g_beacon_cfg.uwb_stslen = g_pending_beacon_cfg.uwb_stslen;
    changed = true;
  }
  if (g_pending_beacon_cfg.uwb_pdoa > 0 && g_pending_beacon_cfg.uwb_pdoa <= 3) {
    config.pdoaMode = pdoa_map[g_pending_beacon_cfg.uwb_pdoa];
    g_beacon_cfg.uwb_pdoa = g_pending_beacon_cfg.uwb_pdoa;
    changed = true;
  }

  if (!changed)
    return;

  uint32_t
      __attribute__((aligned(4))) buffer[(sizeof(flash_dwt_data_t) + 3) / 4];
  memset(buffer, 0, sizeof(buffer));
  flash_dwt_data_t *p_data = (flash_dwt_data_t *)buffer;
  p_data->magic = DWT_CONFIG_MAGIC;
  memcpy(&p_data->cfg, &config, sizeof(dwt_config_t));

  p_data->uwb_options[0] = g_beacon_cfg.uwb_chan;
  p_data->uwb_options[1] = g_beacon_cfg.uwb_plen;
  p_data->uwb_options[2] = g_beacon_cfg.uwb_pac;
  p_data->uwb_options[3] = g_beacon_cfg.uwb_txcode;
  p_data->uwb_options[4] = g_beacon_cfg.uwb_rxcode;
  p_data->uwb_options[5] = g_beacon_cfg.uwb_sfdtype;
  p_data->uwb_options[6] = g_beacon_cfg.uwb_datarate;
  p_data->uwb_options[7] = g_beacon_cfg.uwb_phrmode;
  p_data->uwb_options[8] = g_beacon_cfg.uwb_phrrate;
  p_data->uwb_options[9] = g_beacon_cfg.uwb_sfdto;
  p_data->uwb_options[10] = g_beacon_cfg.uwb_stsmode;
  p_data->uwb_options[11] = g_beacon_cfg.uwb_stslen;
  p_data->uwb_options[12] = g_beacon_cfg.uwb_pdoa;

  if (memcmp((void *)FLASH_START_ADDR, buffer, sizeof(flash_dwt_data_t)) == 0) {
    return; // Already identical
  }

  ret_code_t err_code;
  m_flash_busy = true;
  err_code = sd_flash_page_erase(FLASH_PAGE_NUM);
  if (err_code == NRF_SUCCESS) {
    while (m_flash_busy) {
    } // Wait for event
  } else {
    m_flash_busy = false;
    printf("[Flash] Save: Erase failed error: %lu\n", err_code);
    return;
  }

  m_flash_busy = true;
  err_code =
      sd_flash_write((uint32_t *)FLASH_START_ADDR, buffer, sizeof(buffer) / 4);
  if (err_code == NRF_SUCCESS) {
    while (m_flash_busy) {
    } // Wait for event
  } else {
    m_flash_busy = false;
    printf("[Flash] Save: Write failed error: %lu\n", err_code);
    return;
  }
  printf("[Flash] Saved NEW config to flash\n");
}
