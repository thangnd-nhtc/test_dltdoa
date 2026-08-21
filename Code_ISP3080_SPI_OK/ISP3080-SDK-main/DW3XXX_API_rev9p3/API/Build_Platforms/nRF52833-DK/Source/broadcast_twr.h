#ifndef BROADCAST_TWR_H__
#define BROADCAST_TWR_H__

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief  Khởi tạo module Broadcast TWR (Quản lý Queue Tag)
 */
void bcast_twr_init(void);

/**
 * @brief  Bật/tắt chế độ Broadcast TWR
 */
void bcast_twr_enable(void);
void bcast_twr_disable(void);
bool bcast_twr_is_enabled(void);

/**
 * @brief  Xử lý khi nhận được gói BLE ACK từ Tag trong quá trình Broadcast
 * @param  p_tag_id_raw Trỏ tới 5 bytes ID Tag (BCD)
 * @param  rssi RSSI của gói ACK để lọc theo ngưỡng
 */
void bcast_twr_on_ack_received(const uint8_t *p_tag_id_raw, int8_t rssi);

/**
 * @brief  Lấy Tag ID tiếp theo để thực hiện đo UWB theo round-robin
 * @param  p_tag_id_out Buffer 5 bytes để chứa ID Tag lấy được
 * @return true nếu lấy được Tag, false nếu chưa có Tag phù hợp
 */
bool bcast_twr_next_tag(uint8_t *p_tag_id_out);

/**
 * @brief  Kết thúc lượt đo Tag hiện tại, chỉ cập nhật thống kê, không xóa Tag
 */
void bcast_twr_finish_current(bool success);

/**
 * @brief  API tương thích cũ: kết thúc lượt đo thành công, không xóa Tag nữa
 */
void bcast_twr_remove_current(void);

/**
 * @brief  Tiến hành xử lý scan/cleanup Queue định kỳ (gọi trong main loop)
 */
void bcast_twr_process(void);

#endif // BROADCAST_TWR_H__
