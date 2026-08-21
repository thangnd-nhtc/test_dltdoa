#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "ble_beacon.h"
#include "app_error.h"
#include "ble_advdata.h"
#include "nrf_sdh_ble.h"

static ble_gap_adv_params_t m_adv_params;
static uint8_t              m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static uint8_t              m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
static ble_gap_adv_data_t   m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = NULL,
        .len    = 0
    }
};

void ble_beacon_init(void)
{
    uint32_t      err;
    ble_advdata_t advdata;
    uint8_t       flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

    ble_advdata_manuf_data_t mfg;
    uint8_t data[] = {
        0x02, 0x15, // iBeacon prefix
        0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf0, // UUID
        0x01, 0x02, // Major
        0x03, 0x04, // Minor
        0xC3        // TX Power
    };
    mfg.company_identifier = 0x0059;
    mfg.data.p_data = data;
    mfg.data.size   = sizeof(data);

    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type             = BLE_ADVDATA_NO_NAME;
    advdata.flags                 = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
    advdata.p_manuf_specific_data = &mfg;

    err = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err);

    memset(&m_adv_params, 0, sizeof(m_adv_params));
    m_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    m_adv_params.p_peer_addr     = NULL;
    m_adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval        = 160; // 100ms (160 * 0.625ms)
    m_adv_params.duration        = 0;

    err = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
    APP_ERROR_CHECK(err);
}

void ble_beacon_start(void)
{
    uint32_t err = sd_ble_gap_adv_start(m_adv_handle, BLE_BEACON_CONN_CFG_TAG);
    APP_ERROR_CHECK(err);
}
