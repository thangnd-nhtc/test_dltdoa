#ifndef BLE_BEACON_H__
#define BLE_BEACON_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble_advdata.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_BEACON_CONN_CFG_TAG      1

void ble_beacon_init(void);
void ble_beacon_start(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_BEACON_H__ */
