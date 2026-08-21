#ifndef DUCTHANG_FLASH_H
#define DUCTHANG_FLASH_H

#include "DW3000.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

// ==== Flash persistence ====
#define FLASH_START_ADDR 0x7E000
#define FLASH_PAGE_NUM (FLASH_START_ADDR / 4096)
#define DWT_CONFIG_MAGIC 0xDECA0003

typedef struct {
  uint32_t magic;
  dwt_config_t cfg;
  uint16_t uwb_options[13]; // Lưu các option (1~...) của Web Server
} flash_dwt_data_t;

// Extern variables
extern beacon_cfg_t g_beacon_cfg;
extern beacon_cfg_t g_pending_beacon_cfg;
extern bool flag_save_config_pending;
extern dwt_config_t config;

void load_dwt_config(void);
void cache_pending_dwt_config(void);
void apply_and_save_dwt_config(void);

#endif // DUCTHANG_FLASH_H
