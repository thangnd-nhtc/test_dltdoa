#ifndef SS_TWR_DUCTHANG_H
#define SS_TWR_DUCTHANG_H

#include <stdbool.h>
#include <stdint.h>

/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 100

/* Default antenna delay values for 64 MHz PRF. */
#ifndef TX_ANT_DLY
#define TX_ANT_DLY 16385
#endif
#ifndef RX_ANT_DLY
#define RX_ANT_DLY 16385
#endif

/* Constants for UWB calculations */
#ifndef DWT_TIME_UNITS
#define DWT_TIME_UNITS (1.0 / 499.2e6 / 128.0) //!< = 15.65e-12 s
#endif

/* Length of the common part of the message (up to and including the function
 * code). */
#define ALL_MSG_COMMON_LEN 10

/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14

#define TWR_BUFFER_LENGHT 5
#define CALIB_TWR 0.0001

bool ss_twr_ducthang_handle(void);
float ss_twr_ducthang_get_distance(void);
void ss_twr_ducthang_cleanup(void);

#endif // SS_TWR_DUCTHANG_H
