#ifndef OTA_ISP_H
#define OTA_ISP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/*─────────────────────────────────────────────────────────
 *  OTA SPI Commands  (gửi từ U7→ISP3080 qua SPI Slave)
 *
 *  Tất cả CMD response: m_tx_buf[0] = CMD | 0x80
 *                        m_tx_buf[1] = status (0x00=OK)
 *─────────────────────────────────────────────────────────*/
#define CMD_OTA_ENTER   0x20   /* Payload: fw_size(4B) + crc32(4B)  */
#define CMD_OTA_DATA    0x21   /* Payload: offset(4B) + len(2B) + data[len] */
#define CMD_OTA_END     0x22   /* Không payload – yêu cầu verify     */
#define CMD_OTA_ABORT   0x23   /* Không payload – huỷ OTA             */
#define CMD_OTA_STATUS  0x24   /* Không payload – hỏi trạng thái      */

/* Response status codes */
#define OTA_STATUS_OK           0x00
#define OTA_STATUS_ERR_PARAMS   0x01
#define OTA_STATUS_ERR_ERASE    0x02
#define OTA_STATUS_ERR_WRITE    0x03
#define OTA_STATUS_ERR_CRC      0x04
#define OTA_STATUS_ERR_SIZE     0x05
#define OTA_STATUS_ERR_STATE    0x06
#define OTA_STATUS_BUSY         0x07

/*─────────────────────────────────────────────────────────
 *  Trạng thái máy OTA trên ISP3080
 *─────────────────────────────────────────────────────────*/
typedef enum {
    OTA_STATE_IDLE = 0,     /* Chạy bình thường (TDOA/BLE)  */
    OTA_STATE_READY,        /* Đã nhận ENTER, chờ data       */
    OTA_STATE_RECEIVING,    /* Đang nhận chunk                */
    OTA_STATE_VERIFYING,    /* Verify CRC                     */
    OTA_STATE_COMPLETE,     /* OTA thành công, chờ reboot     */
    OTA_STATE_FAILED,       /* OTA thất bại                   */
} ota_state_t;

/*─────────────────────────────────────────────────────────
 *  Flash Layout cho ISP3080 (nRF52833, 512KB, S140)
 *
 *  0x00000 – 0x26FFF  SoftDevice s140 v7.2  (~156KB)
 *  0x27000 – 0x4FFFF  Application Bank 0    (~164KB max)
 *  0x50000 – 0x7DFFF  OTA Staging Area      (184KB = 46 pages)
 *  0x7E000 – 0x7EFFF  DWT Config (ducthang_flash)
 *  0x7F000 – 0x7FFFF  Bootloader settings / reserved
 *
 *  => Max firmware = 164KB cho staging area (do giới hạn Bank 0).
 *  Nếu file bin của bạn ~110-130KB thì cấu hình này hoàn toàn OK.
 *─────────────────────────────────────────────────────────*/
#define OTA_STAGING_START   0x50000U
#define OTA_STAGING_END     0x7DFFFU   /* Inclusive */
#define OTA_STAGING_SIZE    (OTA_STAGING_END - OTA_STAGING_START + 1)  /* 184KB */
#define OTA_PAGE_SIZE       4096U
#define OTA_STAGING_PAGES   (OTA_STAGING_SIZE / OTA_PAGE_SIZE)  /* 46 pages */


/* Max chunk size gửi qua SPI (BUF_LEN=512, trừ header) */
#define OTA_MAX_CHUNK_SIZE  480U

/*─────────────────────────────────────────────────────────
 *  OTA context (internal, dùng trong ota_isp.c)
 *─────────────────────────────────────────────────────────*/
typedef struct {
    ota_state_t state;
    uint32_t    fw_size;        /* Kích thước firmware tổng     */
    uint32_t    fw_crc32;       /* CRC32 firmware mong đợi      */
    uint32_t    bytes_written;  /* Số byte đã ghi thành công    */
    uint32_t    last_offset;    /* Offset gần nhất ghi xong     */
    uint8_t     last_error;     /* Mã lỗi gần nhất              */
} ota_context_t;

/*─────────────────────────────────────────────────────────
 *  API dành cho main.c  (gọi từ SPI switch-case)
 *─────────────────────────────────────────────────────────*/

/**
 * @brief  Khởi tạo module OTA (gọi 1 lần trong main trước vòng lặp).
 */
void ota_isp_init(void);

/**
 * @brief  Xử lý lệnh CMD_OTA_ENTER.
 *         Parse fw_size + crc32 từ SPI rx buffer.
 *         Erase staging area.
 *         Set ota_mode = 1.
 *
 * @param  rx_buf   Con trỏ đến m_rx_copy (bao gồm byte CMD ở [0])
 * @param  rx_len   Số byte nhận được
 * @param  tx_buf   Con trỏ đến m_tx_buf để ghi phản hồi
 * @return Số byte response cần gửi (ít nhất 2)
 */
uint16_t ota_handle_enter(const uint8_t *rx_buf, size_t rx_len, uint8_t *tx_buf);

/**
 * @brief  Xử lý lệnh CMD_OTA_DATA.
 *         Parse offset + len + data từ SPI rx buffer.
 *         Ghi vào staging flash.
 *
 * @param  rx_buf   Con trỏ đến m_rx_copy
 * @param  rx_len   Số byte nhận được
 * @param  tx_buf   Con trỏ đến m_tx_buf
 * @return Số byte response
 */
uint16_t ota_handle_data(const uint8_t *rx_buf, size_t rx_len, uint8_t *tx_buf);

/**
 * @brief  Xử lý lệnh CMD_OTA_END.
 *         Verify CRC32 toàn bộ staging area.
 *         Nếu OK, sẽ copy sang Bank 0 và reboot.
 *
 * @param  tx_buf   Con trỏ đến m_tx_buf
 * @return Số byte response
 */
uint16_t ota_handle_end(uint8_t *tx_buf);

/**
 * @brief  Xử lý lệnh CMD_OTA_ABORT.
 *         Huỷ OTA, quay về IDLE.
 *
 * @param  tx_buf   Con trỏ đến m_tx_buf
 * @return Số byte response
 */
uint16_t ota_handle_abort(uint8_t *tx_buf);

/**
 * @brief  Xử lý lệnh CMD_OTA_STATUS.
 *         Trả về state + bytes_written + last_error.
 *
 * @param  tx_buf   Con trỏ đến m_tx_buf
 * @return Số byte response
 */
uint16_t ota_handle_status(uint8_t *tx_buf);

/**
 * @brief  Kiểm tra xem hệ thống đang ở chế độ OTA không.
 * @return true nếu đang OTA (TDOA/BLE cần dừng)
 */
bool ota_isp_is_active(void);

/**
 * @brief  Lấy trạng thái OTA hiện tại.
 */
ota_state_t ota_isp_get_state(void);

/**
 * @brief  Gọi trong main loop nếu OTA hoàn tất (state=COMPLETE).
 *         Thực hiện copy staging → Bank 0 rồi reboot.
 *         Hàm này KHÔNG return nếu thành công (NVIC_SystemReset).
 */
void ota_isp_apply_and_reboot(void);

#endif /* OTA_ISP_H */
