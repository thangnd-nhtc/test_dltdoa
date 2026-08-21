/**
 * @file    ota_isp.c
 * @brief   OTA firmware update qua SPI cho ISP3080 (nRF52833 + S140)
 *
 * Sử dụng sd_flash_page_erase / sd_flash_write (SoftDevice-safe)
 * giống cách ducthang_flash.c đang dùng.
 *
 * Flow:
 *   1. CMD_OTA_ENTER  → erase staging area, set state=READY
 *   2. CMD_OTA_DATA   → ghi chunk vào staging flash
 *   3. CMD_OTA_END    → verify CRC32, nếu OK → copy to Bank0 → reboot
 *   4. CMD_OTA_ABORT  → huỷ, quay về IDLE
 */

#include "ota_isp.h"
#include "main.h"
#include "nrf_sdh_soc.h"
#include "nrf_soc.h"
#include "nrf_delay.h"
#include "nrf_nvic.h"
#include <string.h>

/*─────────────────────────────────────────────────────────
 *  Flash busy flag (dùng chung pattern với ducthang_flash.c)
 *─────────────────────────────────────────────────────────*/
static volatile bool m_ota_flash_busy = false;

static void ota_soc_evt_handler(uint32_t evt_id, void *p_context) {
    if (evt_id == NRF_EVT_FLASH_OPERATION_SUCCESS ||
        evt_id == NRF_EVT_FLASH_OPERATION_ERROR) {
        m_ota_flash_busy = false;
    }
}
NRF_SDH_SOC_OBSERVER(m_ota_soc_observer, 1, ota_soc_evt_handler, NULL);

/*─────────────────────────────────────────────────────────
 *  OTA context
 *─────────────────────────────────────────────────────────*/
static ota_context_t m_ota_ctx;

/*─────────────────────────────────────────────────────────
 *  CRC32 (polynomial 0xEDB88320, same as zlib/Ethernet)
 *─────────────────────────────────────────────────────────*/
static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320U;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}

/*─────────────────────────────────────────────────────────
 *  Helpers: flash erase / write qua SoftDevice
 *─────────────────────────────────────────────────────────*/

/** Erase 1 page (4KB). Blocking (chờ SoftDevice event). */
static bool ota_flash_erase_page(uint32_t page_num) {
    ret_code_t err;
    m_ota_flash_busy = true;
    err = sd_flash_page_erase(page_num);
    if (err != NRF_SUCCESS) {
        m_ota_flash_busy = false;
        printf("[OTA] Erase page %lu failed: %lu\n", page_num, err);
        return false;
    }
    /* Chờ SoftDevice báo xong */
    uint32_t timeout = 0;
    while (m_ota_flash_busy) {
        if (++timeout > 5000000) {
            printf("[OTA] Erase page %lu timeout\n", page_num);
            return false;
        }
    }
    return true;
}

/**
 * Ghi dữ liệu vào flash. Data phải word-aligned.
 * @param addr   Địa chỉ flash (phải chia hết cho 4)
 * @param data   Dữ liệu ghi (phải word-aligned)
 * @param len    Số byte (phải chia hết cho 4)
 */
static bool ota_flash_write(uint32_t addr, const uint32_t *data, size_t len_bytes) {
    ret_code_t err;
    uint32_t words = (len_bytes + 3) / 4;

    m_ota_flash_busy = true;
    err = sd_flash_write((uint32_t *)addr, data, words);
    if (err != NRF_SUCCESS) {
        m_ota_flash_busy = false;
        printf("[OTA] Write 0x%08lX failed: %lu\n", addr, err);
        return false;
    }
    uint32_t timeout = 0;
    while (m_ota_flash_busy) {
        if (++timeout > 5000000) {
            printf("[OTA] Write 0x%08lX timeout\n", addr);
            return false;
        }
    }
    return true;
}

/*─────────────────────────────────────────────────────────
 *  API Implementation
 *─────────────────────────────────────────────────────────*/

void ota_isp_init(void) {
    memset(&m_ota_ctx, 0, sizeof(m_ota_ctx));
    m_ota_ctx.state = OTA_STATE_IDLE;
    printf("[OTA] Module initialized\n");
}

bool ota_isp_is_active(void) {
    return (m_ota_ctx.state != OTA_STATE_IDLE);
}

ota_state_t ota_isp_get_state(void) {
    return m_ota_ctx.state;
}

/*─────────────────────────────────────────────────────────
 *  CMD_OTA_ENTER  (0x20)
 *  rx_buf: [CMD(1)] [fw_size(4, LE)] [crc32(4, LE)]
 *  Total: 9 bytes minimum
 *─────────────────────────────────────────────────────────*/
uint16_t ota_handle_enter(const uint8_t *rx_buf, size_t rx_len, uint8_t *tx_buf) {
    /* Đặt giá trị mặc định là BUSY (0xFE) để phần cứng SPIS (DMA) không báo sớm OK (0xA0) khi U7 thăm dò trong lúc đang xóa flash */
    tx_buf[0] = 0xFE;
    tx_buf[1] = 0xFE;

    /* Chỉ cho phép ENTER khi đang IDLE */
    if (m_ota_ctx.state != OTA_STATE_IDLE) {
        printf("[OTA] ENTER rejected: state=%d\n", m_ota_ctx.state);
        tx_buf[1] = OTA_STATUS_ERR_STATE;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    /* Kiểm tra payload */
    if (rx_len < 9) {
        printf("[OTA] ENTER: payload too short (%d)\n", rx_len);
        tx_buf[1] = OTA_STATUS_ERR_PARAMS;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    /* Parse fw_size (4 byte LE) + crc32 (4 byte LE) */
    uint32_t fw_size = (uint32_t)rx_buf[1]
                     | ((uint32_t)rx_buf[2] << 8)
                     | ((uint32_t)rx_buf[3] << 16)
                     | ((uint32_t)rx_buf[4] << 24);

    uint32_t fw_crc  = (uint32_t)rx_buf[5]
                     | ((uint32_t)rx_buf[6] << 8)
                     | ((uint32_t)rx_buf[7] << 16)
                     | ((uint32_t)rx_buf[8] << 24);

    /* Kiểm tra kích thước */
    if (fw_size == 0 || fw_size > OTA_STAGING_SIZE) {
        printf("[OTA] ENTER: invalid size %lu (max %lu)\n", fw_size, (uint32_t)OTA_STAGING_SIZE);
        tx_buf[1] = OTA_STATUS_ERR_SIZE;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    printf("[OTA] ENTER: fw_size=%lu, crc32=0x%08lX\n", fw_size, fw_crc);

    /* Erase staging area (chỉ erase số page cần thiết) */
    uint32_t pages_needed = (fw_size + OTA_PAGE_SIZE - 1) / OTA_PAGE_SIZE;
    printf("[OTA] Erasing %lu pages (with SD-safe delays)...\n", pages_needed);

    for (uint32_t i = 0; i < pages_needed; i++) {
        uint32_t page_num = (OTA_STAGING_START / OTA_PAGE_SIZE) + i;
        if (!ota_flash_erase_page(page_num)) {
            printf("[OTA] Erase failed at page %lu\n", page_num);
            tx_buf[1] = OTA_STATUS_ERR_ERASE;
            memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
            return BUF_LEN;
        }
        /* Cho SoftDevice xử lý BLE events giữa mỗi page erase */
        nrf_delay_ms(2);
        
        /* Đề phòng Watchdog Timer (WDT) đang chạy ở Reload Register khác ngoài 0, ta feed toàn bộ */
        if (NRF_WDT->RUNSTATUS) {
            for (uint8_t r = 0; r < 8; r++) {
                if (NRF_WDT->RREN & (1 << r)) {
                    NRF_WDT->RR[r] = 0x6E524635; // NRF_WDT_RR_VALUE_VALID
                }
            }
        }

        if (i % 10 == 9) {
            printf("[OTA] Erased %lu/%lu pages\n", i + 1, pages_needed);
        }
    }

    printf("[OTA] Erase done. Ready for data.\n");

    /* Cập nhật context */
    m_ota_ctx.state         = OTA_STATE_READY;
    m_ota_ctx.fw_size       = fw_size;
    m_ota_ctx.fw_crc32      = fw_crc;
    m_ota_ctx.bytes_written = 0;
    m_ota_ctx.last_offset   = 0;
    m_ota_ctx.last_error    = 0;

    tx_buf[0] = CMD_OTA_ENTER | 0x80;
    tx_buf[1] = OTA_STATUS_OK;
    memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
    return BUF_LEN;
}

/*─────────────────────────────────────────────────────────
 *  CMD_OTA_DATA  (0x21)
 *  rx_buf: [CMD(1)] [offset(4,LE)] [len(2,LE)] [data[len]]
 *  Total minimum: 1+4+2+1 = 8 bytes
 *─────────────────────────────────────────────────────────*/
uint16_t ota_handle_data(const uint8_t *rx_buf, size_t rx_len, uint8_t *tx_buf) {
    /* Mặc định BUSY */
    tx_buf[0] = 0xFE;
    tx_buf[1] = 0xFE;

    /* State check */
    if (m_ota_ctx.state != OTA_STATE_READY && m_ota_ctx.state != OTA_STATE_RECEIVING) {
        tx_buf[0] = CMD_OTA_DATA | 0x80;
        tx_buf[1] = OTA_STATUS_ERR_STATE;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    if (rx_len < 8) {
        tx_buf[0] = CMD_OTA_DATA | 0x80;
        tx_buf[1] = OTA_STATUS_ERR_PARAMS;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    /* Parse offset + len */
    uint32_t offset = (uint32_t)rx_buf[1]
                    | ((uint32_t)rx_buf[2] << 8)
                    | ((uint32_t)rx_buf[3] << 16)
                    | ((uint32_t)rx_buf[4] << 24);

    uint16_t data_len = (uint16_t)rx_buf[5]
                      | ((uint16_t)rx_buf[6] << 8);

    const uint8_t *data = &rx_buf[7];

    /* Validation */
    if (data_len == 0 || data_len > OTA_MAX_CHUNK_SIZE) {
        tx_buf[0] = CMD_OTA_DATA | 0x80;
        tx_buf[1] = OTA_STATUS_ERR_PARAMS;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    if ((uint32_t)rx_len < 7 + data_len) {
        tx_buf[0] = CMD_OTA_DATA | 0x80;
        tx_buf[1] = OTA_STATUS_ERR_PARAMS;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    if (offset + data_len > m_ota_ctx.fw_size) {
        printf("[OTA] DATA: offset(%lu)+len(%u) > fw_size(%lu)\n",
               offset, data_len, m_ota_ctx.fw_size);
        tx_buf[0] = CMD_OTA_DATA | 0x80;
        tx_buf[1] = OTA_STATUS_ERR_SIZE;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    /* Chuẩn bị buffer word-aligned để ghi flash */
    uint32_t flash_addr = OTA_STAGING_START + offset;

    /* sd_flash_write yêu cầu data phải word-aligned và len phải bội 4 */
    uint32_t padded_len = (data_len + 3) & ~3U;
    uint32_t __attribute__((aligned(4))) aligned_buf[(OTA_MAX_CHUNK_SIZE + 3) / 4];
    memset(aligned_buf, 0xFF, padded_len); /* Pad bằng 0xFF (trạng thái erased) */
    memcpy(aligned_buf, data, data_len);

    /* Ghi flash */
    if (!ota_flash_write(flash_addr, aligned_buf, padded_len)) {
        printf("[OTA] DATA: write failed at 0x%08lX\n", flash_addr);
        m_ota_ctx.last_error = OTA_STATUS_ERR_WRITE;
        tx_buf[0] = CMD_OTA_DATA | 0x80;
        tx_buf[1] = OTA_STATUS_ERR_WRITE;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    /* Cập nhật context */
    m_ota_ctx.state = OTA_STATE_RECEIVING;
    m_ota_ctx.bytes_written = offset + data_len;
    m_ota_ctx.last_offset   = offset;

    /* Response: OK + current bytes_written (để U7 theo dõi progress) */
    tx_buf[0] = CMD_OTA_DATA | 0x80;
    tx_buf[1] = OTA_STATUS_OK;
    tx_buf[2] = (uint8_t)(m_ota_ctx.bytes_written & 0xFF);
    tx_buf[3] = (uint8_t)((m_ota_ctx.bytes_written >> 8) & 0xFF);
    tx_buf[4] = (uint8_t)((m_ota_ctx.bytes_written >> 16) & 0xFF);
    tx_buf[5] = (uint8_t)((m_ota_ctx.bytes_written >> 24) & 0xFF);
    memset(&tx_buf[6], 0xFF, BUF_LEN - 6);
    return BUF_LEN;
}

/*─────────────────────────────────────────────────────────
 *  CMD_OTA_END  (0x22) — Verify CRC32 toàn bộ staging area
 *─────────────────────────────────────────────────────────*/
uint16_t ota_handle_end(uint8_t *tx_buf) {
    tx_buf[0] = 0xFE;
    tx_buf[1] = 0xFE;

    if (m_ota_ctx.state != OTA_STATE_RECEIVING) {
        tx_buf[0] = CMD_OTA_END | 0x80;
        tx_buf[1] = OTA_STATUS_ERR_STATE;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    /* Kiểm tra đã nhận đủ data chưa */
    if (m_ota_ctx.bytes_written < m_ota_ctx.fw_size) {
        printf("[OTA] END: incomplete %lu/%lu bytes\n",
               m_ota_ctx.bytes_written, m_ota_ctx.fw_size);
        tx_buf[1] = OTA_STATUS_ERR_SIZE;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    m_ota_ctx.state = OTA_STATE_VERIFYING;
    printf("[OTA] Verifying CRC32 (%lu bytes)...\n", m_ota_ctx.fw_size);

    /* Tính CRC32 trên staging area */
    const uint8_t *staging = (const uint8_t *)OTA_STAGING_START;
    uint32_t computed_crc = crc32_update(0, staging, m_ota_ctx.fw_size);

    printf("[OTA] CRC32: computed=0x%08lX, expected=0x%08lX\n",
           computed_crc, m_ota_ctx.fw_crc32);

    if (computed_crc != m_ota_ctx.fw_crc32) {
        printf("[OTA] CRC MISMATCH! OTA failed, NOT rebooting.\n");
        m_ota_ctx.state      = OTA_STATE_FAILED;
        m_ota_ctx.last_error = OTA_STATUS_ERR_CRC;
        tx_buf[1] = OTA_STATUS_ERR_CRC;
        memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
        return BUF_LEN;
    }

    printf("[OTA] CRC OK! Firmware verified.\n");
    m_ota_ctx.state = OTA_STATE_COMPLETE;

    tx_buf[1] = OTA_STATUS_OK;
    memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
    return BUF_LEN;
}

/*─────────────────────────────────────────────────────────
 *  CMD_OTA_ABORT  (0x23) — Huỷ OTA
 *─────────────────────────────────────────────────────────*/
uint16_t ota_handle_abort(uint8_t *tx_buf) {
    tx_buf[0] = CMD_OTA_ABORT | 0x80;

    printf("[OTA] ABORT: state was %d\n", m_ota_ctx.state);

    m_ota_ctx.state = OTA_STATE_IDLE;
    m_ota_ctx.bytes_written = 0;
    m_ota_ctx.last_error    = 0;

    tx_buf[1] = OTA_STATUS_OK;
    memset(&tx_buf[2], 0xFF, BUF_LEN - 2);
    return BUF_LEN;
}

/*─────────────────────────────────────────────────────────
 *  CMD_OTA_STATUS  (0x24) — Hỏi trạng thái
 *  Response: [CMD|0x80] [status] [state(1)] [written(4)] [fw_size(4)] [error(1)]
 *─────────────────────────────────────────────────────────*/
uint16_t ota_handle_status(uint8_t *tx_buf) {
    tx_buf[0]  = CMD_OTA_STATUS | 0x80;
    tx_buf[1]  = OTA_STATUS_OK;
    tx_buf[2]  = (uint8_t)m_ota_ctx.state;
    tx_buf[3]  = (uint8_t)(m_ota_ctx.bytes_written & 0xFF);
    tx_buf[4]  = (uint8_t)((m_ota_ctx.bytes_written >> 8) & 0xFF);
    tx_buf[5]  = (uint8_t)((m_ota_ctx.bytes_written >> 16) & 0xFF);
    tx_buf[6]  = (uint8_t)((m_ota_ctx.bytes_written >> 24) & 0xFF);
    tx_buf[7]  = (uint8_t)(m_ota_ctx.fw_size & 0xFF);
    tx_buf[8]  = (uint8_t)((m_ota_ctx.fw_size >> 8) & 0xFF);
    tx_buf[9]  = (uint8_t)((m_ota_ctx.fw_size >> 16) & 0xFF);
    tx_buf[10] = (uint8_t)((m_ota_ctx.fw_size >> 24) & 0xFF);
    tx_buf[11] = m_ota_ctx.last_error;
    memset(&tx_buf[12], 0xFF, BUF_LEN - 12);
    return BUF_LEN;
}

/*─────────────────────────────────────────────────────────
 *  Copy staging → Bank 0 rồi reboot
 *
 *  QUAN TRỌNG:
 *  App đang chạy (Bank 0) KHÔNG thể gọi `sd_flash_page_erase` 
 *  hoặc tự xoá cái Flash chứa code của chính nó. Nếu làm vậy
 *  sẽ bị HardFault / Crash ngay lập tức!!
 *
 *  GIẢI PHÁP: 
 *  Chạy một hàm nhỏ trên RAM (__attribute__((section(".fast")))),
 *  Vô hiệu hoá SoftDevice / Ngắt, 
 *  Gọi trực tiếp thanh ghi NVMC để Erase Bank 0 và Copy toàn bộ code mới,
 *  Sau đó kích hoạt Reset.
 *─────────────────────────────────────────────────────────*/

#define APP_START_ADDR  0x27000U

/* Hàm đặt tại section .fast (chạy trên RAM) */
__attribute__((section(".fast")))
static void ota_ram_copy_and_reboot(uint32_t src_addr, uint32_t dst_addr, uint32_t size) {
    /* 1. Disbale tất cả các ngắt & SoftDevice exceptions để an toàn tuyệt đối */
    __disable_irq();

    uint32_t num_pages = (size + 4095) / 4096;
    uint32_t num_words = (size + 3) / 4;
    uint32_t *p_src = (uint32_t *)src_addr;
    uint32_t *p_dst = (uint32_t *)dst_addr;

    /* Thanh ghi phần cứng NRF_NVMC (Bypass SoftDevice) */
    volatile uint32_t *nvmc_ready   = (volatile uint32_t *)0x4001E400; /* NRF_NVMC->READY */
    volatile uint32_t *nvmc_config  = (volatile uint32_t *)0x4001E504; /* NRF_NVMC->CONFIG */
    volatile uint32_t *nvmc_erase   = (volatile uint32_t *)0x4001E508; /* NRF_NVMC->ERASEPAGE */
    volatile uint32_t *aircr        = (volatile uint32_t *)0xE000ED0C; /* SCB->AIRCR (Reset) */

    /* 2. Erase toàn bộ Bank 0 */
    for (uint32_t i = 0; i < num_pages; i++) {
        *nvmc_config = 0x02; /* CONFIG.WEN = EEN (Erase mode) */
        __ISB(); __DSB();
        *nvmc_erase = (dst_addr + i * 4096);
        while (*nvmc_ready == 0) {} /* Chờ erase xong */
    }

    /* 3. Copy toàn bộ code từ Staging -> Bank 0 */
    *nvmc_config = 0x01; /* CONFIG.WEN = WEN (Write mode) */
    __ISB(); __DSB();
    for (uint32_t i = 0; i < num_words; i++) {
        p_dst[i] = p_src[i];
        while (*nvmc_ready == 0) {} /* Chờ write từng word */
    }

    /* 4. Khóa flash lại */
    *nvmc_config = 0x00; /* CONFIG.WEN = REN (Read mode) */
    __ISB(); __DSB();

    /* 5. Cứng Reset Thiết Bị để chạy firmware mới! */
    *aircr = 0x05FA0004; /* SYSRESETREQ */
    while (1) {} /* Sẽ không bao giờ tới đây */
}

void ota_isp_apply_and_reboot(void) {
    if (m_ota_ctx.state != OTA_STATE_COMPLETE) {
        printf("[OTA] apply_and_reboot: wrong state %d\n", m_ota_ctx.state);
        return;
    }

    uint32_t fw_size = m_ota_ctx.fw_size;

    /* Reset context trước khi copy/reboot để lần OTA kế tiếp luôn bắt đầu từ IDLE.
     * fw_size đã được cache vào biến local nên không ảnh hưởng quá trình copy.
     */
    m_ota_ctx.state = OTA_STATE_IDLE;
    m_ota_ctx.bytes_written = 0;
    m_ota_ctx.last_offset = 0;
    m_ota_ctx.last_error = 0;

    printf("[OTA] RAM-COPY BEGIN: Staging(0x%08lX) -> App(0x%08lX) Size: %lu bytes\n",
           (uint32_t)OTA_STAGING_START, (uint32_t)APP_START_ADDR, fw_size);
    printf("[OTA] WARNING: CPU will block, SoftDevice disabled. Rebooting soon...\n");
    nrf_delay_ms(100); /* Đợi UART in log xong */

    // NHẢY VÀO HÀM RAM, KHÔNG QUAY LẠI CẢ Ở ĐÂY NỮA!
    ota_ram_copy_and_reboot(OTA_STAGING_START, APP_START_ADDR, fw_size);
}
