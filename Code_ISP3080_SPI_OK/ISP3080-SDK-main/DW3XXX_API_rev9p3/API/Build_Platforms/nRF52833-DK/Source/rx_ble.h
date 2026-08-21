#ifndef RX_BLE_H__
#define RX_BLE_H__

#include <stdbool.h>
#include <stdint.h>


/**
 * @brief Initialize the BLE scanning module.
 */
void rx_ble_init(void);

/**
 * @brief Start scanning for advertising packets.
 */
void rx_ble_scan_start(void);

/**
 * @brief Stop scanning.
 */
void rx_ble_scan_stop(void);

#endif // RX_BLE_H__
