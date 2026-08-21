#ifndef _HANDLE_COM_H_
#define _HANDLE_COM_H_

#include "Arduino.h"

#include "define.h"
#include "handle_com_regs.h"
#include "handle_spifs.h"
#include "handle_status.h"

/*** FRAME DATA COMMUNICATION
 * 1 Byte: loại giao tiếp (đọc cấu hình, ghi cấu hình)
 * 1 Byte: check_crc. tính từ sau byte check_crc đến hết
 * 2 Byte: địa chỉ, thể hiện vị trí config, hoặc giao tiếp data
 * 2 Byte: độ dài data
 * array data
 ****************************/
// extern TimeOutEvent ESPRebootTo;
extern dw_two_way_t two_way;
void distant(double Distance);
// send_twr_result() đã deprecated — dùng SPI→TCP port 2011

typedef struct {
  struct {
    uint8_t type;
    uint16_t check_crc;
    mask_regs_t msk_regs;
  } header;
  uint8_t data[COMMUNICATION_LENGHT_BUFFER];
} frame_work_t;

#define SIZE_FRAME_COMWORK sizeof(frame_work_t)

class ComHandle {
public:
  ComHandle(/* args */);
  ~ComHandle();

  void GiveBuff(uint8_t *data, uint8_t length);
  int TakeBuff(uint8_t *data);

  int8_t process_raw_data(frame_work_t *data);
  void loop_polling_fragment();
  frame_work_t com_frame;

private:
  struct {
    volatile uint8_t id_rd;
    volatile uint8_t id_wr;
    struct {
      uint8_t length;
      uint8_t array[COMMUNICATION_LENGHT_BUFFER];
    } data[COMMUNICATION_NUM_BUFFER];
  } com_buff;

  // chương trình xử lý riêng dành cho thư viện
  bool CheckBuff(void);
  void process_write_ram(mask_regs_t *regs, uint8_t *data);
};

extern ComHandle Handle_Com;

#endif
