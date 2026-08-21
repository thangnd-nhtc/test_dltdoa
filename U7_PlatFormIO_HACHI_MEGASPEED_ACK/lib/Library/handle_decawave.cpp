#include "handle_decawave.h"
#include "TimeOutEvent.h"
#include "WiFi.h"
#include "config.h"
#include "handle_ISP3080.h"
#include "main.h"

// PTB
volatile bool nrf_irq_flag = false; // set trong ISR
void IRAM_ATTR nrf_irq_isr()
{
  nrf_irq_flag = true;
  flag_isp3080 = true;
}
// PTB

TimeOutEvent checktimeERROR(3000);

bool FlagtimeERROR = false;

bool FlagReadconfig = true;

DwHandle::DwHandle(/* args */) { decawave_recheck_to = new TimeOutEvent(5000); }
DwHandle::~DwHandle() {}

void DwHandle::reloadDW()
{
  if (FlagReadconfig)
  {
    FlagReadconfig = false;
    this->parameters();
  }
}

void DwHandle::parameters(void)
{

  if (!parametter_dw.read(FILE_PARAMETTER_DW, MASTER_RES.add,
                          (uint8_t *)&parametter_dw.master))
  {
    //	memset((uint8_t *)&parametter_dw.master, 0, sizeof(master_t));
  }

  if (!parametter_dw.read(FILE_PARAMETTER_DW, MASTER_ACCESS_RES1.add,
                          (uint8_t *)&parametter_dw.master_access[0]))
  {
    dbg_dw("read MASTER_ACCESS_RES1 FAIL");
    memset((uint8_t *)&parametter_dw.master_access[0], 0,
           sizeof(master_access_t));
  }

  if (!parametter_dw.read(FILE_PARAMETTER_DW, MASTER_ACCESS_RES2.add,
                          (uint8_t *)&parametter_dw.master_access[1]))
  {
    dbg_dw("read MASTER_ACCESS_RES2 FAIL");
    memset((uint8_t *)&parametter_dw.master_access[1], 0,
           sizeof(master_access_t));
  }

  if (!parametter_dw.read(FILE_PARAMETTER_DW, MASTER_ACCESS_RES3.add,
                          (uint8_t *)&parametter_dw.master_access[2]))
  {
    dbg_dw("read MASTER_ACCESS_RES3 FAIL");
    memset((uint8_t *)&parametter_dw.master_access[2], 0,
           sizeof(master_access_t));
  }

  if (!parametter_dw.read(FILE_PARAMETTER_DW, MASTER_ACCESS_RES4.add,
                          (uint8_t *)&parametter_dw.master_access[3]))
  {
    dbg_dw("read MASTER_ACCESS_RES4 FAIL");
    memset((uint8_t *)&parametter_dw.master_access[3], 0,
           sizeof(master_access_t));
  }

  if (!parametter_dw.read(FILE_PARAMETTER_DW, DW_CONFIG_RES.add,
                          (uint8_t *)&parametter_dw.dw_config))
  {
    memset((uint8_t *)&parametter_dw.dw_config, 0, sizeof(dw_config_t));
  }

  if (!parametter_dw.read(FILE_PARAMETTER_DW, DW_CONFIG_TX_RES.add,
                          (uint8_t *)&parametter_dw.dw_txconfig))
  {
    memset((uint8_t *)&parametter_dw.dw_txconfig, 0, sizeof(dw_txconfig_t));
  }

  if (!parametter_dw.read(FILE_PARAMETTER_DW, DW_ANT_DELAY_RES.add,
                          (uint8_t *)&parametter_dw.anten_delay))
  {
    memset((uint8_t *)&parametter_dw.anten_delay, 0, sizeof(anten_delay_t));
  }

  // 20240622

  uint32_t dev_id = Config::getDeviceID();
  dbg_dw("Device ID: %lu\n", (unsigned long)dev_id);

  // Gán vào struct của bạn
  parametter_dw.serial_id.device = dev_id;

  //
  this->interval_sync.nomal = TIME_SYNC_1MS * parametter_dw.master.interval;
  this->interval_sync.min = TIME_SYNC_1MS * (parametter_dw.master.interval -
                                             parametter_dw.master.time_prepare);
  this->interval_sync.max = TIME_SYNC_1MS * (parametter_dw.master.interval - 1);

  // dbg_dw("Device ID: %d", parametter_dw.serial_id.device);
  dbg_dw("Device ID: %d \r\n", parametter_dw.serial_id.device);
  // dbg_dw("Broascast ID: %d", parametter_dw.serial_id.broadcast);

  // dbg_dw("Dw config - chan %d", parametter_dw.dw_config.chan);
  // dbg_dw("Dw config - prf %d", parametter_dw.dw_config.prf);
  // dbg_dw("Dw config - txPreambLength %d",
  // parametter_dw.dw_config.txPreambLength); dbg_dw("Dw config - rxPAC %d",
  // parametter_dw.dw_config.rxPAC); dbg_dw("Dw config - txCode %d",
  // parametter_dw.dw_config.txCode); dbg_dw("Dw config - rxCode %d",
  // parametter_dw.dw_config.rxCode); dbg_dw("Dw config - nsSFD %d",
  // parametter_dw.dw_config.nsSFD); dbg_dw("Dw config - dataRate %d",
  // parametter_dw.dw_config.dataRate); dbg_dw("Dw config - phrMode %d",
  // parametter_dw.dw_config.phrMode); dbg_dw("Dw config - sfdTO %d",
  // parametter_dw.dw_config.sfdTO);

  // dbg_dw("Dw config tx - PGdly %d", parametter_dw.dw_txconfig.PGdly);
  // dbg_dw("Dw config tx - power %lu", parametter_dw.dw_txconfig.power);

  // dbg_dw_twr("Dw anten tx delay %d", parametter_dw.anten_delay.tx);
  // dbg_dw_twr("Dw anten rx delay %d", parametter_dw.anten_delay.rx);

  // dbg_dw_sync("master - interval %lu", parametter_dw.master.interval);
  // dbg_dw_sync("master - time_prepare %d", parametter_dw.master.time_prepare);
  // dbg_dw_sync("master - enable %d", parametter_dw.master.enable);

  // dbg_dw_sync("interval sync - nomal %llu", this->interval_sync.nomal);
  // dbg_dw_sync("interval sync - min %llu", this->interval_sync.min);
  // dbg_dw_sync("interval sync - max %llu", this->interval_sync.max);

  // dbg_dw_sync("enable sync1 - %d", parametter_dw.master_access[0].enable);
  // dbg_dw_sync("serial sync1 - %lu", parametter_dw.master_access[0].Serial);
  // dbg_dw_sync("Timestamp sync1 - %llu",
  // parametter_dw.master_access[0].Timestamp);

  // dbg_dw_sync("enable sync2 - %d", parametter_dw.master_access[1].enable);
  // dbg_dw_sync("serial sync2 - %lu", parametter_dw.master_access[1].Serial);
  // dbg_dw_sync("Timestamp sync2 - %llu",
  // parametter_dw.master_access[1].Timestamp);

  // dbg_dw_sync("enable sync3 - %d", parametter_dw.master_access[2].enable);
  // dbg_dw_sync("serial sync3 - %lu", parametter_dw.master_access[2].Serial);
  // dbg_dw_sync("Timestamp sync3 - %llu",
  // parametter_dw.master_access[2].Timestamp);

  // dbg_dw_sync("enable sync4 - %d", parametter_dw.master_access[3].enable);
  // dbg_dw_sync("serial sync4 - %lu", parametter_dw.master_access[3].Serial);
  // dbg_dw_sync("Timestamp sync4 - %llu \r\n",
  // parametter_dw.master_access[3].Timestamp);
}

void handle_led()
{
  if (digitalRead(LOL) || digitalRead(LOS))
  {
    // if (stat_btn)
    {
      led_status.decawace(led_clock_error_DW);
    }
    FlagtimeERROR = true;
    checktimeERROR.ToEUpdate(3000);
    dbg_dw_sync("LED DW LOL-LOS ERROR");
  }
  else
  {
    // if (stat_btn)
    {
      led_status.decawace(led_ok_DW);
    }
    dbg_dw_sync("LED DW LOL-LOS OK");
  }
}

void DwHandle::checkResetDW()
{
  if (FlagtimeERROR)
  {
    if (checktimeERROR.ToEExpired() && (digitalRead(LOL) || digitalRead(LOS)))
    {
      dbg_dw("___RESET DW____");
      digitalWrite(RESET_CLKDW, LOW);
      delay(10);
      digitalWrite(RESET_CLKDW, HIGH);
      FlagtimeERROR = false;
    }
    else if (checktimeERROR.ToEExpired())
    {
      FlagtimeERROR = false;
    }
  }
}
void reset_module_jitter(void)
{
  // PTB
  pinMode(jitter_pin, OUTPUT);      // Reset DW3000
  pinMode(sync_enable_pin, OUTPUT); // Power switch (nếu có)
  // pinMode(rst_isp3080, OUTPUT);
  // digitalWrite(sync_enable_pin, HIGH);
  // digitalWrite(jitter_pin, LOW);
  // delay(10);
  digitalWrite(sync_enable_pin, LOW);
  digitalWrite(jitter_pin, HIGH);
  delay(10);
}

bool DwHandle::begin(void)
{
  parametter_dw.del(FILE_PARAMETTER_DW);
  // set time recheck decawave
  this->set_recheck();
  this->mode = mode_wait;
  this->decawace_ready_f = false;
  FlagReadconfig = true;

  // PTB
  reset_module_jitter();
  // PTB
  reset_isp3080();
  // PTB

  attachInterrupt(digitalPinToInterrupt(LOL), handle_led, CHANGE);
  attachInterrupt(digitalPinToInterrupt(LOS), handle_led, CHANGE);

  // PTB
  // Khởi tạo SPI giao tiếp với ISP3080 (nRF52833) dùng HSPI
  isp3080_spi_init(8000000); // SPI @ 4MHz, Mode 0

  pinMode(PIN_IRQ_FROM_NRF, INPUT); // PULL-DOWN không cần – GPIO35 input only
  attachInterrupt(digitalPinToInterrupt(PIN_IRQ_FROM_NRF), nrf_irq_isr,
                  RISING); // bắt cạnh lên RISING/FALLING
  //  PTB
  // PTB
  delay(10);
  if (begin_dw3000())
  {
    // dbg_dw("Init DW Ok");
    last_quiet_ms = millis(); // Cập nhật ngay sau khi init thành công
    this->set_recheck();
  }
  else
  {
    // bat co bao loi
    this->decawace_ready_f = false;
    dbg_dw("Init DW Error");
    // led status

    // if (stat_btn)
    {
      led_status.decawace(led_error_DW);
    }

    return false;
  }
  // PTB

  /*load parameters*/ // error
  if (FlagReadconfig)
  {
    FlagReadconfig = false;
    this->parameters();
  }

  delay(10);
  set_device_id_to_nrf(
      parametter_dw.serial_id.device); // Gửi ID thiết bị đến nRF

  parametter_dw.master.interval = 100;

  this->Buffer_En();
  this->Buffer_Index();

  dbg_dw("Init DW Ok");
  dbg_dw("%s", "Init DW Ok");

  // Đọc ISP3080 FW/HW version 1 lần duy nhất lúc boot
  {
    extern char g_isp_fw_ver[10];
    extern char g_isp_hw_ver[10];
    if (read_isp_version(g_isp_fw_ver, sizeof(g_isp_fw_ver), g_isp_hw_ver,
                         sizeof(g_isp_hw_ver)))
    {
      dbg_dw("[BOOT] ISP FW:%s HW:%s\n", g_isp_fw_ver, g_isp_hw_ver);
    }
    else
    {
      dbg_dw("%s", "[BOOT] Khong doc duoc ISP version!");
    }
  }

  delay(10);
  this->reciver_enable(true);

  // bat co bao ready
  this->decawace_ready_f = true;
  // led status
  // if (stat_btn)
  {
    led_status.decawace(led_ok_DW);
  }

  return true;
}

bool DwHandle::begin_nrf(void)
{
  // set time recheck decawave
  this->set_recheck();
  this->decawace_ready_f = false;
  // PTB
  reset_module_jitter();
  // PTB
  reset_isp3080();
  // PTB

  // attachInterrupt(digitalPinToInterrupt(LOL), handle_led, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(LOS), handle_led, CHANGE);

  // PTB
  // Khởi tạo SPI giao tiếp với ISP3080 (nRF52833) dùng HSPI
  // isp3080_spi_init(8000000); // SPI @ 4MHz, Mode 0

  // pinMode(PIN_IRQ_FROM_NRF, INPUT); // PULL-DOWN không cần – GPIO35 input
  // only attachInterrupt(digitalPinToInterrupt(PIN_IRQ_FROM_NRF),
  //				nrf_irq_isr, RISING); // bắt cạnh lên
  // RISING/FALLING
  //   PTB
  //  PTB
  delay(10);
  if (begin_dw3000())
  {
    // dbg_dw("Init DW Ok");
    last_quiet_ms = millis();
    this->set_recheck();
  }
  else
  {
    // bat co bao loi
    this->decawace_ready_f = false;
    dbg_dw("Init DW Error");
    // led status
    led_status.decawace(led_error_DW);
    return false;
  }
  // PTB

  delay(10);
  set_device_id_to_nrf(
      parametter_dw.serial_id.device); // Gửi ID thiết bị đến nRF

  this->Buffer_En();
  this->Buffer_Index();

  dbg_dw("Init DW Ok");
  dbg_dw("%s", "Init DW Ok");

  delay(10);
  this->reciver_enable(true);

  // bat co bao ready
  this->decawace_ready_f = true;
  // led status
  // if (stat_btn)
  {
    led_status.decawace(led_ok_DW);
  }

  return true;
}

void DwHandle::reciver_enable(bool status)
{
  this->reciver_en_flag = status;

  // error
  if (status == true)
  {
    // dwt_setrxtimeout(0);				  //
    // dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    // dwt_rxenable(DWT_START_RX_IMMEDIATE); //
    // dwt_rxenable(DWT_START_RX_IMMEDIATE);

    // PTB
    // enable_rx_mode(); // Bật chế độ nhận dữ liệu
    // PTB
  }
}

void DwHandle::twr_start(uint32_t addr, HandlerFunction handler)
{
  // disable reciver decawave
  // reciver_enable(false);
  this->toway.data.Src = parametter_dw.serial_id.device;
  this->toway.data.Des = addr;
  this->toway.flag = true;
  this->toway_callback = handler;
}

void DwHandle::Buffer_En(void)
{
  // PTB
  enable_rx_mode(); // Bật chế độ nhận dữ liệu
                    // PTB
}

void DwHandle::Buffer_Index(void)
{
  // dwt_rxenable(DWT_START_RX_IMMEDIATE);
  // uint32_t dw_reg;

  // /*cài đặt lại thanh ghi buffer index*/
  // dw_reg = dwt_read32bitreg(SYS_STATUS_ID);

  // if (dw_reg & SYS_STATUS_HSRBP != (dw_reg & SYS_STATUS_ICRBP) >> 1)
  // {
  // 	/*Host Side Receive Buffer Pointer Toggle*/
  // 	if (dw_reg & SYS_STATUS_ICRBP)
  // 	{
  // 		dw_reg = dwt_read32bitreg(SYS_CTRL_ID);
  // 		dw_reg |= SYS_CTRL_HRBT;
  // 	}
  // 	else
  // 	{
  // 		dw_reg = dwt_read32bitreg(SYS_CTRL_ID);
  // 		dw_reg &= ~SYS_CTRL_HRBT;
  // 	}
  // 	dwt_write32bitreg(SYS_CTRL_ID, dw_reg);
  // }
  // /*Cài đặt auto rx enbale*/
  // dw_reg = dwt_read32bitreg(SYS_CTRL_ID);
  // dw_reg |= SYS_CTRL_RXENAB;
  // dwt_write32bitreg(SYS_CTRL_ID, dw_reg);

  // PTB
  // thêm hàm xử lý nếu cần
  // PTB
}

// void DwHandle::check_isp3080(void) {
//   if (this->decawace_ready_f == false)
//     return;
//   // if (this->mode != mode_tdoa)
//   //   return;
//   if (check_ack_isp3080()) {
//     // Nếu nRF52 hoat dong tot, reset timer recheck
//     this->set_recheck();
//   } else {
//     // Chi reset khi thuc su mat ket noi qua lau
//     this->begin_nrf();
//   }
// }
void DwHandle::check_isp3080(void)
{
  if (this->decawace_ready_f == false)
    return;
  // Nếu đang SS-TWR (mode 5) thì không cần kiểm tra mất kết nối nRF52
  if (temp_id_mode == 5)
  {
    return;
  }

  // Grace period: khi vừa thoát mode 5 (SS-TWR), cho nRF52 thời gian chuyển
  // mode Tránh reset ISP3080 trong khi nRF52 đang xử lý BLE config mới
  static uint8_t prev_mode = 0;
  static unsigned long mode_change_ms = 0;
  if (prev_mode == 5 && temp_id_mode != 5)
  {
    mode_change_ms = millis(); // Ghi nhận thời điểm chuyển mode
  }
  prev_mode = temp_id_mode;

  if (mode_change_ms > 0 && (millis() - mode_change_ms < 500))
  {
    return; // Chờ 500ms cho nRF52 xử lý xong SPI config và dừng TWR
  }
  mode_change_ms = 0;

  // Nếu không ở chế độ TDOA thì không cần kiểm tra
  if (this->mode != mode_tdoa)
    return;

  if (check_ack_isp3080())
  {
    // ISP3080 vẫn sống (trả lời SPI), làm mới timer recheck để tránh reset
    // reset linh tinh khi không có Tag
    this->set_recheck();
  }
  else
  {
    // Thưc sự không phản hồi SPI mới reset
    this->begin_nrf();
    // reset_module_jitter();
    // reset_isp3080();
    // begin_dw3000();
  }
}

bool DwHandle::Check_TX(int status)
{
  uint16_t counter_retry = 0;
  // If returns an error, abandon this ranging exchange and proceed to the next
  // one.
  if (status != DWT_SUCCESS)
    return false;

  // Chờ cho decawave phát thành công
  while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
  {
    if (counter_retry++ > 500)
    {
      dwt_forcetrxoff(); // hàm này dw3000 giống như dw1000
      // led_status.decawace(led_sync_ERROR_DW);
      dbg_dw("TX error timeout");
      return false;
    }
    delayMicroseconds(100);
  }

  dbg_dw("TX OK");

  // set time recheck decawave
  // Set lại timeout reset dw3000 khi phát gói tin offset
  this->set_recheck();
  return true;
}

/*Read buffer*/
int8_t DwHandle::reciver_data_raw(uint8_t *data, uint32_t threshold_length,
                                  uint8_t rd_tx_ts, uint8_t rd_rxts)
{
  uint32_t dw_reg;

  // static uint8_t abc=0;
  /*Đọc thanh ghi trạng thái của decawave*/
  dw_reg = dwt_read32bitreg(SYS_STATUS_ID);

  // if(abc == 1)
  // {
  // 	abc = 0;
  // 		dbg_dw("%d", dw_reg);

  // }
  /*overload double buffer*/
  if (dw_reg & SYS_STATUS_RXOVRR)
  {
    // dbg_dw("%d", dw_reg);
    // abc = 1;
    dbg_dw("Error check reciver ...");
    dwt_forcetrxoff(); // hàm này dw3000 giống như dw1000
    dwt_setdblrxbuffmode(
        1); // dwt_setdblrxbuffmode(DBL_BUF_STATE_EN, DBL_BUF_MODE_AUTO);

    this->Buffer_En();
    this->Buffer_Index();
    if (dw_reg & SYS_STATUS_HSRBP == (dw_reg & SYS_STATUS_ICRBP) >> 1)
    {
      /*Mask Double buffered status bits*/
      dw_reg = dwt_read32bitreg(SYS_MASK_ID);
      dw_reg |= (SYS_MASK_MRXFCE | SYS_MASK_MRXFCG | SYS_MASK_MRXDFR |
                 SYS_MASK_MLDEDONE);
      dwt_write32bitreg(SYS_MASK_ID, dw_reg);

      /* Clear good RX frame event in the DW1000 status register. */
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG | SYS_STATUS_RXFCE |
                                           SYS_STATUS_RXDFR |
                                           SYS_STATUS_LDEDONE);

      /*UnMask Double buffered status bits*/
      dw_reg = dwt_read32bitreg(SYS_MASK_ID);
      dw_reg &= ~(SYS_MASK_MRXFCE | SYS_MASK_MRXFCG | SYS_MASK_MRXDFR |
                  SYS_MASK_MLDEDONE);
      dwt_write32bitreg(SYS_MASK_ID, dw_reg);
    }

    /*Host Side Receive Buffer Pointer Toggle*/
    dw_reg = dwt_read32bitreg(SYS_CTRL_ID);
    dw_reg |= SYS_CTRL_HRBT;
    dwt_write32bitreg(SYS_CTRL_ID, dw_reg);
    return -1;
  }
  /*Nếu có dữ liệu RX good*/
  if (dw_reg & SYS_STATUS_RXFCG)
  {
    // dbg_dw("Read check reciver ...");
    //  dbg_dw("%d", dw_reg);
    /* Đọc dữ liệu của decawave*/
    if (rd_tx_ts)
      data_rx_raw.tx_timestamp = this->get_tx_timestamp_u64();
    if (rd_rxts)
      data_rx_raw.rx_timestamp = this->get_rx_timestamp_u64();
    uint32_t RX_Frame_Info = dwt_read32bitreg(RX_FINFO_ID);
    data_rx_raw.length = RX_Frame_Info & RX_FINFO_RXFL_MASK_1023;
    dwt_readrxdata(data_rx_raw.buffer, data_rx_raw.length, 0);

    /* Read diagnostics data. */
    memset((uint8_t *)&data_rx_raw.rx_diag, 0, sizeof(data_rx_raw.rx_diag));
    dwt_readdiagnostics(&data_rx_raw.rx_diag);
    data_rx_raw.Pream_AccCnt = (RX_Frame_Info & RX_FINFO_RXPACC_MASK) >> 20;

    // /*overload double buffer*/
    // dw_reg = dwt_read32bitreg(SYS_STATUS_ID);
    // if (dw_reg & SYS_STATUS_RXOVRR)
    // {
    // 	// dbg_dw("Error reciver...");
    // 	dwt_forcetrxoff();
    // 	this->Buffer_Index();
    // 	return -1;
    // }

    /* cài đặt lại thanh ghi buffer index*/
    if (dw_reg & SYS_STATUS_HSRBP == (dw_reg & SYS_STATUS_ICRBP) >> 1)
    {
      /*Mask Double buffered status bits*/
      dw_reg = dwt_read32bitreg(SYS_MASK_ID);
      dw_reg |= (SYS_MASK_MRXFCE | SYS_MASK_MRXFCG | SYS_MASK_MRXDFR |
                 SYS_MASK_MLDEDONE);
      dwt_write32bitreg(SYS_MASK_ID, dw_reg);

      /* Clear good RX frame event in the DW1000 status register. */
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG | SYS_STATUS_RXFCE |
                                           SYS_STATUS_RXDFR |
                                           SYS_STATUS_LDEDONE);

      /*UnMask Double buffered status bits*/
      dw_reg = dwt_read32bitreg(SYS_MASK_ID);
      dw_reg &= ~(SYS_MASK_MRXFCE | SYS_MASK_MRXFCG | SYS_MASK_MRXDFR |
                  SYS_MASK_MLDEDONE);
      dwt_write32bitreg(SYS_MASK_ID, dw_reg);
    }

    /*Host Side Receive Buffer Pointer Toggle*/
    dw_reg = dwt_read32bitreg(SYS_CTRL_ID);
    dw_reg |= SYS_CTRL_HRBT;
    dwt_write32bitreg(SYS_CTRL_ID, dw_reg);

    /* Giai ma */
    if (data_rx_raw.length <= threshold_length)
    {
      memset(data, 0, threshold_length);
      HW_AES_CBC128_2B_decryption(data, data_rx_raw.buffer);
      return 1;
    }
  }
  return 0;
}

void DwHandle::reciver(void)
{
  // static uint8_t index_buffer = 1;

  // nếu không cấu hình được dw thì bỏ qua
  if (this->decawace_ready_f == false)
    return;

  // Nếu decawave bận làm việc khác thì bỏ qua
  // if (this->reciver_en_flag == false)
  // 	return;

  // PTB
  if (nrf_irq_flag)
  {
    this->set_recheck();
    nrf_irq_flag = false;

    uint8_t frame_mode = read_frame_from_nrf();
    switch (frame_mode)
    {
    case SPI_FRAME_TYPE_SERVER:
      // this->set_recheck();
      //  Xử lý server frame
      this->mode = mode_tdoa;
      // dbg_dw("go to mode TDoA");
      break;

    case SPI_FRAME_TYPE_DW:
      // this->set_recheck();
      //  Xử lý DW frame
      this->mode = mode_twr;
      dbg_dw("go to mode TWR");
      // Forward distance to U6 if in Mode 5
      // BLOCK: Khi BSS-TWR đang bật, không gửi DS-TWR result qua distant()
      // vì sẽ gây spam CMD 8 với giá trị giống BSS-TWR
      if (frame_twr.Cmd == Cmd_Distance && temp_id_mode == 5 &&
          !g_beacon_cfg.enable_bcast_twr)
      {
        if (frame_twr.Data.DIST.Dis == UINT32_MAX)
        {
          distant(-1.0);
        }
        else
        {
          double dist_m = (double)frame_twr.Data.DIST.Dis / 1000.0;
          distant(dist_m);
        }
      }
      break;

    case SPI_FRAME_TYPE_OFFSET:

      // this->set_recheck();
      //  Xử lý offset frame
      memcpy(&frame_offset1, &frame_ofsset_rx, sizeof(dw_dataframe_t));
      this->mode = mode_offset_rx;
      dbg_dw("go to mode Offset RX");
      break;

    case 0xFF:
    default:
      // Lỗi hoặc không xác định
      break;
    }
  }
  // PTB

  // uint8_t rx_buffer[SIZE_OF_DATAFRAME + 8] = {0};
  //  Đọc thanh ghi trạng thái nhận của dw1000
  // int8_t dw_read_flag = this->reciver_data_raw(rx_buffer, SIZE_OF_DATAFRAME +
  // 2, 1, 1);
  //  if (dw_read_flag == 1)
  //  {
  //  	// set time recheck decawave
  // 	this->set_recheck();

  // 	// Lấy dữ liệu vào struct
  // 	(index_buffer == 1) ? (index_buffer = 0) : (index_buffer = 1);
  // 	// Lấy dữ liệu vào struct
  // 	memcpy((uint8_t *)&this->data_rx[index_buffer], rx_buffer,
  // SIZE_OF_DATAFRAME);

  // 	// Debug
  // 	// dbg_dw_rx("Des: %u", this->data_rx.Des);
  // 	// dbg_dw_rx("Src: %u", this->data_rx.Src);
  // 	// dbg_dw_rx("Packit: %u", this->data_rx.Packet_Id);
  // 	// dbg_dw_rx("Cmd: %d", this->data_rx.Cmd);

  // 	// Serial.println("");
  // 	// for (uint8_t i = 0; i < SIZE_OF_DATAFRAME; i++)
  // 	// 	Serial.printf("%d-%d\r\n", i, rx_buffer[i]);

  // 	switch ((dw_command)this->data_rx[index_buffer].Cmd)
  // 	{
  // 	case Cmd_Tag: // Nếu mã lệnh là Tag
  // 	{
  // 		// xác thực địa chỉ
  // 		if (this->data_rx[index_buffer].Des !=
  // parametter_dw.serial_id.broadcast && this->data_rx[index_buffer].Des !=
  // parametter_dw.serial_id.device) 			break;
  // frame_tag = &data_rx[index_buffer];

  // 		// dbg_dw_tdoa("pin %d",this->frame_tag->Data.TAG.Battery);

  // 		this->mode = mode_tdoa;

  // 		// dbg_dw_rx("go to mode TDoA");
  // 		break;
  // 	}
  // 	case Cmd_Sync: // Nếu mã lệnh là Sync
  // 	{
  // 		// xác thực địa chỉ
  // 		if (this->data_rx[index_buffer].Des !=
  // parametter_dw.serial_id.broadcast && this->data_rx[index_buffer].Des !=
  // parametter_dw.serial_id.device) 			break;

  // 		frame_sync = &data_rx[index_buffer];
  // 		this->mode = mode_sync_rx;

  // 		dbg_dw_rx("go to mode Sync RX");
  // 		break;
  // 	}
  // 	case Cmd_Offset: // Nếu mã lệnh là offset
  // 	{
  // 		// xác thực địa chỉ
  // 		if (this->data_rx[index_buffer].Des !=
  // parametter_dw.serial_id.broadcast && this->data_rx[index_buffer].Des !=
  // parametter_dw.serial_id.device) 			break;

  // 		// frame_offset = &data_rx[index_buffer];
  // 		memcpy(&frame_offset1, &this->data_rx[index_buffer],
  // sizeof(this->data_rx[index_buffer])); 		this->mode =
  // mode_offset_rx;

  // 		// dbg_dw_rx("go to mode Offset RX");
  // 		break;
  // 	}
  // 	default: // nếu không phải 2 lệnh trên
  // 	{
  // 		if (this->data_rx[index_buffer].Cmd != Cmd_Pool &&
  // 			this->data_rx[index_buffer].Cmd != Cmd_Resp &&
  // 			this->data_rx[index_buffer].Cmd != Cmd_Final &&
  // 			this->data_rx[index_buffer].Cmd != Cmd_Distance)
  // 			break;
  // 		// xác thực địa chỉ
  // 		if (this->data_rx[index_buffer].Des !=
  // parametter_dw.serial_id.device) 			break;

  // 		memcpy((uint8_t *)&this->toway.data, (uint8_t
  // *)&data_rx[index_buffer], SIZE_OF_DATAFRAME); 		this->mode =
  // mode_twr;

  // 		// dbg_dw_rx("goto mode Toway");
  // 		break;
  // 	}
  // 	}
  // }
  // else if (dw_read_flag == -1)
  // {
  // 	// led status
  // 	// led_status.decawace(led_error_DW);

  // 	// // Reset RX to properly reinitialise LDE operation.
  // 	// dwt_rxreset();
  // 	// dbg_dw("--------- error dwt_rx reset ---------");
  // 	// led_status.decawace(led_ok_DW);
  // }
}

// Old_Tag lod_tag[30];
// uint8_t demTag = 0;
/*TDOA*/
void DwHandle::tdoa(void)
{
  if (this->mode != mode_tdoa)
    return;
  this->mode = mode_wait;

  char csv_buffer[255];
  int csv_len = build_csv_from_server_frame(&server_frame_rx, csv_buffer,
                                            sizeof(csv_buffer));
  if (csv_len > 0)
  {
    Handle_Com.GiveBuff((uint8_t *)csv_buffer, csv_len);
    this->set_recheck();
  }
  // Serial.printf("[DEBUG] Sent CSV Frame (%d bytes): %s", csv_len,
  // csv_buffer); Serial.printf("%s", csv_buffer);

  // double C, N, RX_Level;
  // C = this->data_rx_raw.rx_diag.maxGrowthCIR * pow(2, 17);
  // N = pow(this->data_rx_raw.Pream_AccCnt, 2);
  // RX_Level = 10 * log10(C / N) - 121.74;
  // RX_Level = abs(RX_Level);

  /* Tao du lieu gui truoc ma hoa */

  // switch (this->frame_tag->Data.TAG.Type)
  // {
  // case Cmd_tag_nomal: // type data = 0
  // {
  // 	this->data_transmits.Type_data = Cmd_tag_nomal;

  // 	this->value2array(this->data_transmits.Packit_ID,
  // this->frame_tag->Packet_Id, 4);
  // 	this->value2array(this->data_transmits.Serial_ID,
  // parametter_dw.serial_id.device, 4);
  // 	this->value2array(this->data_transmits.Timestamp,
  // this->data_rx_raw.rx_timestamp, 5); 	for (uint8_t i = 0; i <
  // DECAWAVE_MASTER_ACCESS_NUM; i++)
  // 	{
  // 		this->value2array(this->data_transmits.Mts_access[i].Serial,
  // parametter_dw.master_access[i].Serial, 4);
  // 		this->value2array(this->data_transmits.Mts_access[i].Timestamp,
  // parametter_dw.master_access[i].Timestamp, 8);
  // 		this->value2array(this->data_transmits.Mts_access[i].Packit_ID,
  // this->frame_offset1.Packet_Id, 4);
  // 	}
  // 	this->value2array(this->data_transmits.Tag_ID, this->frame_tag->Src, 4);
  // 	this->data_transmits.Motion = this->frame_tag->Data.TAG.Motion;
  // 	this->data_transmits.Button = this->frame_tag->Data.TAG.Button;
  // 	this->data_transmits.Free_fall = this->frame_tag->Data.TAG.Battery;
  // 	this->data_transmits.RSSI = (uint8_t)RX_Level;

  // 	break;
  // }
  // case Cmd_tag_sensor: // type data = 1 //ETAG
  // {
  // 	dbg_dw_tdoa("-1-> %u,%u,%u",
  // 				this->frame_tag->Data.TAG.Custom.ETAG.Compass,
  // 				this->frame_tag->Data.TAG.Custom.ETAG.Pressure,
  // 				this->frame_tag->Data.TAG.Custom.ETAG.Acceleration);

  // 	this->data_transmits.Type_data = Cmd_tag_sensor;

  // 	this->value2array(this->data_transmits.Packit_ID,
  // this->frame_tag->Packet_Id, 4);
  // 	this->value2array(this->data_transmits.Serial_ID,
  // parametter_dw.serial_id.device, 4);
  // 	this->value2array(this->data_transmits.Timestamp,
  // this->data_rx_raw.rx_timestamp, 5); 	for (uint8_t i = 0; i <
  // DECAWAVE_MASTER_ACCESS_NUM; i++)
  // 	{
  // 		this->value2array(this->data_transmits.Mts_access[i].Serial,
  // parametter_dw.master_access[i].Serial, 4);
  // 		this->value2array(this->data_transmits.Mts_access[i].Timestamp,
  // parametter_dw.master_access[i].Timestamp, 8);
  // 		this->value2array(this->data_transmits.Mts_access[i].Packit_ID,
  // this->frame_offset1.Packet_Id, 4);
  // 	}
  // 	this->value2array(this->data_transmits.Tag_ID, this->frame_tag->Src, 4);
  // 	this->data_transmits.Motion = this->frame_tag->Data.TAG.Motion;
  // 	this->data_transmits.Button = this->frame_tag->Data.TAG.Button;
  // 	this->data_transmits.Free_fall = this->frame_tag->Data.TAG.Battery;
  // 	this->data_transmits.RSSI = (uint8_t)RX_Level;

  // 	// thêm cho type 1
  // 	this->value2array(this->data_transmits.Type1.Compass,
  // this->frame_tag->Data.TAG.Custom.ETAG.Compass, 4);
  // 	this->value2array(this->data_transmits.Type1.Pressure,
  // this->frame_tag->Data.TAG.Custom.ETAG.Pressure, 4);
  // 	this->value2array(this->data_transmits.Type1.Accelermeter,
  // this->frame_tag->Data.TAG.Custom.ETAG.Acceleration, 4);

  // 	break;
  // }
  // case Cmd_tag_solut: // SOLUT TAG
  // {
  // 	dbg_dw_tdoa("-2-> %u,%u,%u",
  // 				this->frame_tag->Data.TAG.Custom.SOLUT.Temper,
  // 				this->frame_tag->Data.TAG.Custom.SOLUT.Humi,
  // 				this->frame_tag->Data.TAG.Custom.SOLUT.Vibra);

  // 	this->data_transmits.Type_data = Cmd_tag_solut;

  // 	this->value2array(this->data_transmits.Packit_ID,
  // this->frame_tag->Packet_Id, 4); // ID Tag
  // 	this->value2array(this->data_transmits.Serial_ID,
  // parametter_dw.serial_id.device, 4);
  // 	this->value2array(this->data_transmits.Timestamp,
  // this->data_rx_raw.rx_timestamp, 5); 	for (uint8_t i = 0; i <
  // DECAWAVE_MASTER_ACCESS_NUM; i++)
  // 	{
  // 		this->value2array(this->data_transmits.Mts_access[i].Serial,
  // parametter_dw.master_access[i].Serial, 4);
  // 		this->value2array(this->data_transmits.Mts_access[i].Timestamp,
  // parametter_dw.master_access[i].Timestamp, 8);
  // 		this->value2array(this->data_transmits.Mts_access[i].Packit_ID,
  // this->frame_offset1.Packet_Id, 4);
  // 	}
  // 	this->value2array(this->data_transmits.Tag_ID, this->frame_tag->Src, 4);
  // 	this->data_transmits.Motion = this->frame_tag->Data.TAG.Motion;
  // // Điện áp 	this->data_transmits.Button =
  // this->frame_tag->Data.TAG.Button;
  // // Dòng điện 	this->data_transmits.Free_fall =
  // this->frame_tag->Data.TAG.Battery; // Nhiệt độ this->data_transmits.RSSI =
  // (uint8_t)RX_Level;

  // 	this->data_transmits.Type2.Temperature =
  // this->frame_tag->Data.TAG.Custom.SOLUT.Temper; // % Battery
  // 	this->data_transmits.Type2.Humidity =
  // this->frame_tag->Data.TAG.Custom.SOLUT.Humi;		// Charge Status
  // 	this->data_transmits.Type2.Vibrate =
  // this->frame_tag->Data.TAG.Custom.SOLUT.Vibra;

  // 	break;
  // }
  // case Cmd_tag_dps422: // DPS422 TAG
  // {
  // 	dbg_dw_tdoa("-4-> %u,%u",
  // 				this->frame_tag->Data.TAG.Custom.DPS422.Temper,
  // 				this->frame_tag->Data.TAG.Custom.DPS422.Pressure);

  // 	this->data_transmits.Type_data = Cmd_tag_dps422;

  // 	this->value2array(this->data_transmits.Packit_ID,
  // this->frame_tag->Packet_Id, 4);
  // 	this->value2array(this->data_transmits.Serial_ID,
  // parametter_dw.serial_id.device, 4);
  // 	this->value2array(this->data_transmits.Timestamp,
  // this->data_rx_raw.rx_timestamp, 5); 	for (uint8_t i = 0; i <
  // DECAWAVE_MASTER_ACCESS_NUM; i++)
  // 	{
  // 		this->value2array(this->data_transmits.Mts_access[i].Serial,
  // parametter_dw.master_access[i].Serial, 4);
  // 		this->value2array(this->data_transmits.Mts_access[i].Timestamp,
  // parametter_dw.master_access[i].Timestamp, 8);
  // 		this->value2array(this->data_transmits.Mts_access[i].Packit_ID,
  // this->frame_offset1.Packet_Id, 4);
  // 	}
  // 	this->value2array(this->data_transmits.Tag_ID, this->frame_tag->Src, 4);
  // 	this->data_transmits.Motion = this->frame_tag->Data.TAG.Motion;
  // 	this->data_transmits.Button = this->frame_tag->Data.TAG.Button;
  // 	this->data_transmits.Free_fall = this->frame_tag->Data.TAG.Battery;
  // 	this->data_transmits.RSSI = (uint8_t)RX_Level;

  // 	this->value2array(this->data_transmits.Type3.Temperature,
  // this->frame_tag->Data.TAG.Custom.DPS422.Temper, 2);
  // 	this->value2array(this->data_transmits.Type3.Pressure,
  // this->frame_tag->Data.TAG.Custom.DPS422.Pressure, 4);

  // 	break;
  // }
  // }

  // 		Handle_Com.GiveBuff((uint8_t *)&this->data_transmits,
  // sizeof(server_dataframe_t));
  // 		dbg_dw_tdoa("%d,%lu,%lu,%llu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%d,%d,%d,%d",
  // 		this->frame_tag->Data.TAG.Type,
  // 		this->frame_tag->Packet_Id,
  // 		parametter_dw.serial_id.device,
  // 		this->data_rx_raw.rx_timestamp,
  // 		parametter_dw.master_access[0].Serial,
  // 		parametter_dw.master_access[0].Timestamp,
  // 		parametter_dw.master_access[1].Serial,
  // 		parametter_dw.master_access[1].Timestamp,
  // 		parametter_dw.master_access[2].Serial,
  // 		parametter_dw.master_access[2].Timestamp,
  // 		parametter_dw.master_access[3].Serial,
  // 		parametter_dw.master_access[3].Timestamp,
  // 		this->frame_tag->Src,
  // 		this->frame_tag->Data.TAG.Motion,
  // 		this->frame_tag->Data.TAG.Button,
  // 		this->frame_tag->Data.TAG.Battery,
  // 		(uint8_t)RX_Level);

  // for (int i = 0; i < 30; i++)
  // {
  // 	if (this->frame_tag->Packet_Id == 0)
  // 	{
  // 		if (lod_tag[i].Serial_ID == this->frame_tag->Src)
  // 			lod_tag[i].Serial_ID = 0;
  // 	}

  // 	else if (lod_tag[i].Packet_Id >= this->frame_tag->Packet_Id &&
  // (lod_tag[i].Serial_ID == this->frame_tag->Src))
  // 	{
  // 		dbg_dw_tdoa("_______duplicate TAG_______:%lu/%lu-%lu",
  // lod_tag[i].Packet_Id, this->frame_tag->Packet_Id, this->frame_tag->Src);
  // 		return;
  // 	}
  // }
  // lod_tag[demTag].Packet_Id = this->frame_tag->Packet_Id;
  // lod_tag[demTag].Serial_ID = this->frame_tag->Src;
  // demTag++;
  // if (demTag >= 30)
  // 	demTag = 0;

  // this->frame_tag->Packet_Id;
  // char buffer[255];
  // memset((char *)buffer, NULL, 255);
  // int len = sprintf(buffer,
  // "%d,%lu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%llu,%lu,%d,%d,%d,%d:\n\r\n\r",
  // 				  this->frame_tag->Data.TAG.Type,
  // 				  this->frame_tag->Packet_Id,
  // 				  parametter_dw.serial_id.device,
  // 				  this->data_rx_raw.rx_timestamp,
  // 				  (uint32_t)0, //
  // parametter_dw.master_access[0].Serial, (uint64_t)0, //
  // parametter_dw.master_access[0].Timestamp, (uint32_t)0, //
  // parametter_dw.master_access[1].Serial, (uint64_t)0, //
  // parametter_dw.master_access[1].Timestamp, (uint32_t)0, //
  // parametter_dw.master_access[2].Serial, (uint64_t)0, //
  // parametter_dw.master_access[2].Timestamp, (uint32_t)0, //
  // parametter_dw.master_access[3].Serial, (uint64_t)0, //
  // parametter_dw.master_access[3].Timestamp,
  // this->frame_tag->Src,
  // this->frame_tag->Data.TAG.Motion,
  // this->frame_tag->Data.TAG.Button,
  // this->frame_tag->Data.TAG.Battery,
  // (uint8_t)RX_Level); if (this->frame_tag->Src == 9011)
  // {
  // 	dbg_dw_tdoa("%s", buffer);
  // }

  // }

  // Handle_Com.GiveBuff((uint8_t *)&buffer, len);
  //  if(this->frame_tag->Src == (uint32_t)8033)

  // else
  // dbg_dw_tdoa(" %s",buffer);
}

/*SYNC*/
void DwHandle::sync_tx(void)
{
// nếu không sync qua dw thì bỏ qua chương trình này
#if DECAWAVE_SYNC_AIR == 0
  return;
#endif

  static TimeOutEvent sync_to(0);
  static uint32_t packet_id_sync = 2;
  static uint32_t Sys_time_Tx;

  dw_dataframe_t data_tx;
  uint32_t Systime;
  uint32_t Tsub;
  uint32_t Ctn;
  // nếu không phải là master thì bỏ qua
  if (parametter_dw.master.enable == false)
    return;

  dbg_dw_sync("---------------------------sync_tx------------------------------"
              "-----\r\n");
  // // kiểm tra tới thời điểm phát sync từ time esp
  // sync_to.ToEExpired();
  // if (sync_to.ToEGetStatus() == true)
  // 	return;

  // // set cac thông số mặt định
  // data_tx.Src = parametter_dw.serial_id.device;
  // data_tx.Des = parametter_dw.serial_id.broadcast;
  // data_tx.Cmd = Cmd_Sync;

  // // đọc thời gian timestamp
  // Systime = dwt_readsystimestamphi32();
  // Tsub = Systime - Sys_time_Tx;
  // dbg_dw_sync("SyncTx 2: %u/%u - %u/%u", Systime, Sys_time_Tx, Tsub,
  // this->interval_sync.min);

  // // nếu timestamp chưa tới thời điểm phát sync thì thoát ra
  // if (Tsub < this->interval_sync.min)
  // 	return;
  // dbg_dw_sync("SyncTx 3");
  // // vô hiệu hóa chương trình nhận decawave
  // this->reciver_enable(false);

  // // Nếu thời gian bé hơn interval sync max thì
  // if (Tsub < this->interval_sync.max)
  // {
  // 	dwt_forcetrxoff();

  // 	// tính toán packit id. (có tính năng packit tính theo time)
  // 	Ctn = (1 + Tsub / this->interval_sync.nomal);
  // 	packet_id_sync += Ctn;
  // 	Sys_time_Tx += Ctn * this->interval_sync.nomal;

  // 	data_tx.Packet_Id = packet_id_sync;

  // 	tdoa_set_timestamp_u64((uint64_t)Sys_time_Tx << 8,
  // data_tx.Data.SYNC.Ts);

  // 	/* Set time delay */
  // 	dwt_setdelayedtrxtime(Sys_time_Tx);

  // 	uint8_t array[SIZE_OF_DATAFRAME + 8];
  // 	uint8_t array_aes[SIZE_OF_DATAFRAME + 10];

  // 	memcpy(array, (uint8_t *)&data_tx, SIZE_OF_DATAFRAME);

  // 	/* ma hoa du lieu */
  // 	HW_AES_CBC128_2B_encryption(array_aes, array);

  // 	dwt_writetxdata(SIZE_OF_DATAFRAME + 2, array_aes, 0); /* Zero offset in
  // TX buffer. */ 	dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2, 0, 0);
  // /* Zero offset in TX buffer, ranging. */

  // 	// dbg_dw_sync("SyncTx,%lu,%lu,%lu,%lu",
  // 	// 			packet_id_sync,
  // 	// 			Sys_time_Tx,
  // 	// 			Systime,
  // 	// 			Tsub);

  // 	if (this->Check_TX(dwt_starttx(DWT_START_TX_DELAYED)) == false)
  // 		dbg_dw_sync("SyncTx Fail :%u", packet_id_sync);
  // 	else
  // 	{
  // 		dbg_dw_sync("SyncTx OK :%u", packet_id_sync);

  // 		// Đọc gói timestamp phát gói tin
  // 		data_rx_raw.rx_timestamp = get_tx_timestamp_u64();

  // 		frame_sync = &data_tx;
  // 		this->mode = mode_sync_rx;
  // 	}
  // 	// Clear TX frame sent event.
  // 	dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

  // 	// Update timeout esp32
  // 	sync_to.ToEUpdate(parametter_dw.master.interval -
  // parametter_dw.master.time_prepare);
  // }
  // // nếu thời gian lớn hơn giá trị interval sync max thì bỏ qua
  // else if (Tsub > this->interval_sync.nomal)
  // {
  // 	Ctn = Tsub / this->interval_sync.nomal;
  // 	packet_id_sync += Ctn;
  // 	Sys_time_Tx += Ctn * this->interval_sync.nomal;
  // 	dbg_dw_sync("interval sync overtime");
  // }

  // this->reciver_enable(true);
}

// void DwHandle::sync_DW(void)
// {

// 	static TimeOutEvent sync_to(0);
// 	static uint32_t packet_id_sync = 2;
// 	static uint32_t Sys_time_Tx;

// 	dw_dataframe_t data_tx;
// 	uint32_t Systime;
// 	uint32_t Tsub;
// 	uint32_t Ctn;
// 	dbg_dw_sync("SyncTx 0");
// 	// nếu không phải là master thì bỏ qua
// 	if (parametter_dw.master.enable == false)
// 		return;
// 	dbg_dw_sync("SyncTx 1");
// 	// kiểm tra tới thời điểm phát sync từ time esp
// 	sync_to.ToEExpired();
// 	if (sync_to.ToEGetStatus() == true)
// 		return;
// 	dbg_dw_sync("SyncTx 2");
// 	// set cac thông số mặt định
// 	data_tx.Src = parametter_dw.serial_id.device;
// 	data_tx.Des = parametter_dw.serial_id.broadcast;
// 	data_tx.Cmd = Cmd_Sync;

// 	// đọc thời gian timestamp
// 	Systime = dwt_readsystimestamphi32();
// 	Tsub = Systime - Sys_time_Tx;
// 	dbg_dw_sync("SyncTx 3");
// 	// nếu timestamp chưa tới thời điểm phát sync thì thoát ra
// 	if (Tsub < this->interval_sync.min)
// 		return;
// 	dbg_dw_sync("SyncTx 4");
// 	// vô hiệu hóa chương trình nhận decawave
// 	this->reciver_enable(false);

// 	// Nếu thời gian bé hơn interval sync max thì
// 	if (Tsub < this->interval_sync.max)
// 	{
// 		dwt_forcetrxoff();

// 		// tính toán packit id. (có tính năng packit tính theo time)
// 		Ctn = (1 + Tsub / this->interval_sync.nomal);
// 		packet_id_sync += Ctn;
// 		Sys_time_Tx += Ctn * this->interval_sync.nomal;

// 		data_tx.Packet_Id = packet_id_sync;

// 		tdoa_set_timestamp_u64((uint64_t)Sys_time_Tx << 8,
// data_tx.Data.SYNC.Ts);

// 		/* Set time delay */
// 		dwt_setdelayedtrxtime(Sys_time_Tx);

// 		uint8_t array[SIZE_OF_DATAFRAME + 8];
// 		uint8_t array_aes[SIZE_OF_DATAFRAME + 10];

// 		memcpy(array, (uint8_t *)&data_tx, SIZE_OF_DATAFRAME);

// 		/* ma hoa du lieu */
// 		HW_AES_CBC128_2B_encryption(array_aes, array);

// 		dwt_writetxdata(SIZE_OF_DATAFRAME + 2, array_aes, 0); /* Zero
// offset in TX buffer. */ 		dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2,
// 0, 0);
// /* Zero offset in TX buffer, ranging. */

// 		// dbg_dw_sync("SyncTx,%lu,%lu,%lu,%lu",
// 		// 			packet_id_sync,
// 		// 			Sys_time_Tx,
// 		// 			Systime,
// 		// 			Tsub);

// 		if (this->Check_TX(dwt_starttx(DWT_START_TX_DELAYED)) == false)
// 			dbg_dw_sync("SyncTx Fail :%u", packet_id_sync);
// 		else
// 		{
// 			dbg_dw_sync("SyncTx OK :%u", packet_id_sync);

// 			// Đọc gói timestamp phát gói tin
// 			data_rx_raw.rx_timestamp = get_tx_timestamp_u64();

// 			frame_sync = &data_tx;
// 			this->mode = mode_sync_rx;
// 		}
// 		// Clear TX frame sent event.
// 		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

// 		// Update timeout esp32
// 		sync_to.ToEUpdate(parametter_dw.master.interval -
// parametter_dw.master.time_prepare);
// 	}
// 	// nếu thời gian lớn hơn giá trị interval sync max thì bỏ qua
// 	else if (Tsub > this->interval_sync.nomal)
// 	{
// 		Ctn = Tsub / this->interval_sync.nomal;
// 		packet_id_sync += Ctn;
// 		Sys_time_Tx += Ctn * this->interval_sync.nomal;
// 		dbg_dw_sync("interval sync overtime");
// 	}

// 	this->reciver_enable(true);
// }

void DwHandle::sync_rx(void)
{
  static uint32_t packet_id = 0;
  if (this->mode != mode_sync_rx)
    return;
  this->mode = mode_wait;

  dbg_dw_sync("---------------------------sync_rx------------------------------"
              "-----\r\n");

  // dbg_dw_sync("sync rx: %d", this->frame_sync->Src);

  // for (uint8_t i = 0; i < DECAWAVE_MASTER_ACCESS_NUM; i++)
  // {
  // 	if (parametter_dw.master_access[i].enable == false)
  // 		continue;

  // 	// dbg_dw_sync("sync enable rx: %d - %d - %lu", i,
  // parametter_dw.master_access[i].enable,
  // parametter_dw.master_access[i].Serial);

  // 	if (parametter_dw.master_access[i].Serial == this->frame_sync->Src)
  // 	{
  // 		parametter_dw.master_access[i].Timestamp =
  // this->data_rx_raw.rx_timestamp; 		return;
  // 	}
  // }
}

/*OFFSET*/
// void DwHandle::tx_offset(void)
// {
// 	static TimeOutEvent tx_offset(0);
// 	static uint32_t packet_id = 0;

// 	dw_dataframe_t data_tx;

// 	// nếu không phải là master thì bỏ qua
// 	if (parametter_dw.master.enable == false)
// 		return;

// 	//
// dbg_dw_sync("---------------------------tx_offset-----------------------------------\r\n");

// 	// kiểm tra tới thời điểm phát sync từ time esp
// 	tx_offset.ToEExpired();
// 	if (tx_offset.ToEGetStatus() == true)
// 		return;

// 	// set cac thông số mặt định
// 	packet_id += 1;
// 	data_tx.Src = parametter_dw.serial_id.device;
// 	data_tx.Des = parametter_dw.serial_id.broadcast;
// 	data_tx.Cmd = Cmd_Offset;
// 	data_tx.Packet_Id = packet_id;

// 	// vô hiệu hóa chương trình nhận decawave
// 	this->reciver_enable(false);

// 	dwt_forcetrxoff();

// 	tdoa_set_timestamp_u64((uint64_t)dwt_readsystimestamphi32() << 8,
// data_tx.Data.SYNC.Ts);

// 	uint8_t array[SIZE_OF_DATAFRAME + 8];
// 	uint8_t array_aes[SIZE_OF_DATAFRAME + 10];

// 	memcpy(array, (uint8_t *)&data_tx, SIZE_OF_DATAFRAME);

// 	/* ma hoa du lieu */
// 	HW_AES_CBC128_2B_encryption(array_aes, array);

// 	dwt_writetxdata(SIZE_OF_DATAFRAME + 2, array_aes, 0); /* Zero offset in
// TX buffer. */ 	dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2, 0, 0);
// /* Zero offset in TX buffer, ranging. */

// 	// dbg_dw_sync("SyncTx,%lu,%lu,%lu,%lu",
// 	// 			packet_id,
// 	// 			Sys_time_Tx,
// 	// 			Systime,
// 	// 			Tsub);

// 	if (this->Check_TX(dwt_starttx(DWT_START_TX_IMMEDIATE)) == false)
// 		dbg_dw_sync("offset tx Fail :%u", packet_id);
// 	else
// 	{
// 		dbg_dw_sync("offset tx OK :%u", packet_id);

// 		// Đọc gói timestamp phát gói tin
// 		data_rx_raw.rx_timestamp = get_tx_timestamp_u64();

// 		// frame_offset = &data_tx;
// 		memcpy(&frame_offset1, &data_tx, sizeof(data_tx));
// 		this->mode = mode_offset_rx;
// 	}
// 	// Clear TX frame sent event.
// 	dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

// 	// Update timeout esp32
// 	tx_offset.ToEUpdate(parametter_dw.master.interval);

// 	this->reciver_enable(true);
// }
// void DwHandle::tx_offset(void)
// {
// 	static TimeOutEvent tx_offset(0);
// 	static uint32_t packet_id = 0;

// 	dw_dataframe_t data_tx;

// 	// Nếu không phải master thì bỏ qua
// 	if (parametter_dw.master.enable == false)
// 		return;

// 	// Kiểm tra timeout chu kỳ phát OFFSET
// 	tx_offset.ToEExpired();
// 	if (tx_offset.ToEGetStatus() == true)
// 		return;

// 	// Tăng packet_id và gán các thông tin cơ bản
// 	packet_id += 1;
// 	data_tx.Src = parametter_dw.serial_id.device;
// 	data_tx.Des = parametter_dw.serial_id.broadcast;
// 	data_tx.Cmd = Cmd_Offset;
// 	data_tx.Packet_Id = packet_id;

// 	// Ghi timestamp giả định (thay cho dwt_readsystimestamphi32)
// 	uint64_t mock_ts = millis(); // dùng thời gian hệ thống làm mô phỏng
// timestamp 	tdoa_set_timestamp_u64(mock_ts << 8, data_tx.Data.SYNC.Ts);

// 	// In ra nội dung gói tin OFFSET
// 	Serial.println("===== [MOCK TX OFFSET FRAME] =====");
// 	Serial.printf("Src        : %lu\n", data_tx.Src);
// 	Serial.printf("Des        : %lu\n", data_tx.Des);
// 	Serial.printf("Packet_Id  : %lu\n", data_tx.Packet_Id);
// 	Serial.printf("Cmd        : %u\n", data_tx.Cmd);
// 	Serial.printf("Timestamp  : %02X %02X %02X %02X %02X\n",
// 				  data_tx.Data.SYNC.Ts[0],
// 				  data_tx.Data.SYNC.Ts[1],
// 				  data_tx.Data.SYNC.Ts[2],
// 				  data_tx.Data.SYNC.Ts[3],
// 				  data_tx.Data.SYNC.Ts[4]);
// 	Serial.println("===================================");

// 	// Cập nhật timeout để phát OFFSET lần sau
// 	tx_offset.ToEUpdate(parametter_dw.master.interval);
// }
void DwHandle::tx_offset(void)
{
  if (this->decawace_ready_f == false)
    return;

  static TimeOutEvent tx_offset(0);
  static uint32_t packet_id = 0;

  dw_dataframe_t data_tx;

  if (!parametter_dw.master.enable)
    return;

  tx_offset.ToEExpired();
  if (tx_offset.ToEGetStatus())
    return;

  packet_id += 1;
  data_tx.Src = parametter_dw.serial_id.device;
  data_tx.Des = parametter_dw.serial_id.broadcast;
  data_tx.Cmd = Cmd_Offset;
  data_tx.Packet_Id = packet_id;

  // Timestamp giả định
  uint64_t mock_ts = millis();
  tdoa_set_timestamp_u64(mock_ts << 8, data_tx.Data.SYNC.Ts);

  // // Debug frame
  // Serial.println("===== [MOCK TX OFFSET FRAME] =====");
  // Serial.printf("Src        : %lu\n", data_tx.Src);
  // Serial.printf("Des        : %lu\n", data_tx.Des);
  // Serial.printf("Packet_Id  : %lu\n", data_tx.Packet_Id);
  // Serial.printf("Cmd        : %u\n", data_tx.Cmd);
  // Serial.printf("Timestamp  : %02X %02X %02X %02X %02X\n",
  //               data_tx.Data.SYNC.Ts[0], data_tx.Data.SYNC.Ts[1],
  //               data_tx.Data.SYNC.Ts[2], data_tx.Data.SYNC.Ts[3],
  //               data_tx.Data.SYNC.Ts[4]);
  // Serial.println("===================================");
  // Note: Nếu bắt đầu phát gói Offset thì clear bộ đếm timeout recheck DW3000
  this->set_recheck(); // 12/12/2025 PTB
  // Gửi bằng send_cmd_to_isp3080
  uint8_t resp[sizeof(dw_dataframe_t) + 4] = {0};
  bool ok = send_cmd_to_isp3080(CMD_SEND_OFFSET,        // cmd = 0x07
                                (uint8_t *)&data_tx,    // payload
                                sizeof(dw_dataframe_t), // payload_len
                                resp,                   // response
                                sizeof(resp)            // response_len (có thể 1-2 byte)
  );

  // Serial.print("[DEBUG] CMD_SEND_OFFSET response: ");
  // for (int i = 0; i < sizeof(resp); i++)
  // {
  // 	Serial.printf("0x%02X ", resp[i]);
  // }
  // Serial.println();

  // if (ok && resp[0] == (CMD_SEND_OFFSET | 0x80) && resp[1] == STATUS_OK)
  // {
  // 	dw_dataframe_t *frame = (dw_dataframe_t *)&resp[4];
  // 	Serial.println("[INFO] Gói OFFSET hợp lệ:");
  // 	Serial.printf("  Cmd       : 0x%02X\n", frame->Cmd);
  // 	Serial.printf("  Src       : 0x%06lX\n", frame->Src);
  // 	Serial.printf("  Des       : 0x%06lX\n", frame->Des);
  // 	Serial.printf("  Packet ID : %d\n", frame->Packet_Id);
  // 	Serial.printf("  Timestamp : %02X %02X %02X %02X %02X\n",
  // 				  frame->Data.SYNC.Ts[0],
  // frame->Data.SYNC.Ts[1], frame->Data.SYNC.Ts[2], frame->Data.SYNC.Ts[3],
  // 				  frame->Data.SYNC.Ts[4]);
  // }
  // else
  // {
  // 	Serial.println("[ERR] Phản hồi không hợp lệ hoặc lỗi trong quá trình
  // gửi.");
  // }

  // if (ok && resp[1] == 0x00)
  // {
  // 	Serial.println("✅ OFFSET đã được gửi cho nRF thành công!");
  // }
  // else
  // {
  // 	Serial.printf("❌ Gửi OFFSET thất bại, resp = %02X %02X\n", resp[0],
  // resp[1]);
  // }

  // memcpy(&frame_offset1, &data_tx, sizeof(data_tx));
  // this->mode = mode_offset_rx;

  tx_offset.ToEUpdate(parametter_dw.master.interval);
}

Old_Tag lod_syn[30];
uint8_t demsyn = 0;

void DwHandle::handle_ducthang_twr()
{
  if (temp_id_mode != 5)
    return;

  static unsigned long last_poll_ms = 0;
  extern bool g_bcast_twr_active;

  if (g_bcast_twr_active)
  {
    // VỚI CƠ CHẾ PUSH MỚI QUA IRQ, KẾT QUẢ ĐƯỢC ĐẨY SANG HÀM
    // read_frame_from_nrf() Giống hệt cơ chế xử lý ACK, không cần poll tay ở
    // đây nữa!
    return;
  }

  // ==== DS-TWR mode bình thường (BSS-TWR tắt) ====
  if (millis() - last_poll_ms >= 500)
  {
    last_poll_ms = millis();
    uint8_t resp[12];
    if (send_cmd_to_isp3080(CMD_GET_SS_TWR_DISTANCE, NULL, 0, resp,
                            sizeof(resp), 100))
    {
      if (resp[1] == 0x00 && resp[3] == 4)
      {
        float dist_f;
        memcpy(&dist_f, &resp[4], 4);
        if (dist_f > 0 && dist_f < 100)
        {
          dbg_dw("DucThang Dist: %.3f m", dist_f);
          uint32_t tid = g_beacon_cfg.val_id_last[0] |
                         (g_beacon_cfg.val_id_last[1] << 8) |
                         (g_beacon_cfg.val_id_last[2] << 16) |
                         (g_beacon_cfg.val_id_last[3] << 24);
          two_way.deviceID = tid;
          distant((double)dist_f);
        }
        else if (dist_f < 0)
        {
          dbg_dw("DucThang: Tag not found (dist=%.1f)", dist_f);
          uint32_t tid = g_beacon_cfg.val_id_last[0] |
                         (g_beacon_cfg.val_id_last[1] << 8) |
                         (g_beacon_cfg.val_id_last[2] << 16) |
                         (g_beacon_cfg.val_id_last[3] << 24);
          two_way.deviceID = tid;
          distant(-1.0);
        }
      }
    }
  }
}

void DwHandle::offset_rx(void)
{
  static uint32_t packet_id;
  if (this->mode != mode_offset_rx)
    return;
  this->mode = mode_wait;

  dbg_dw_sync("---------------------------offset_rx----------------------------"
              "-------\r\n");

  uint32_t SRC = this->frame_offset->Src;
  dbg_dw_sync("offset  rx: %lu,%lu", this->frame_offset1.Src,
              this->frame_offset1.Packet_Id);

  for (uint8_t i = 0; i < DECAWAVE_MASTER_ACCESS_NUM; i++)
  {
    if (parametter_dw.master_access[i].enable == false)
      continue;

    // dbg_dw_sync("sync enable rx: %d - %d - %lu", i,
    // parametter_dw.master_access[i].enable,
    // parametter_dw.master_access[i].Serial);

    if (parametter_dw.master_access[i].Serial == this->frame_offset1.Src)
    {
      for (int i = 0; i < 30; i++)
      {
        if (this->frame_offset1.Packet_Id == 0)
        {
          if (lod_syn[i].Serial_ID == this->frame_offset1.Src)
            lod_syn[i].Serial_ID = 0;
        }

        else if (lod_syn[i].Packet_Id >= this->frame_offset1.Packet_Id &&
                 (lod_syn[i].Serial_ID == this->frame_offset1.Src))
        {
          dbg_dw_tdoa("_______duplicate SYNC_______:%lu/%lu-%lu",
                      lod_syn[i].Packet_Id, this->frame_offset1.Packet_Id,
                      this->frame_offset1.Src);
          return;
        }
      }

      lod_syn[demsyn].Packet_Id = this->frame_offset1.Packet_Id;
      lod_syn[demsyn].Serial_ID = this->frame_offset1.Src;
      demsyn++;
      if (demsyn >= 30)
        demsyn = 0;

      /// uint64_t ts_offset = parse_ts_40bit(frame_offset1.Data.SYNC.Ts);
      // uint64_t ts_offset = 0;
      // memcpy(&ts_offset, frame_offset1.Data.SYNC.Ts, 5);

      char buffer[100];
      memset(buffer, 0, sizeof(buffer));
      int len = sprintf(buffer, "5,%lu,%lu,%lu,%llu:\n\r\n\r",
                        this->frame_offset1.Packet_Id,
                        parametter_dw.serial_id.device, this->frame_offset1.Src,
                        parse_ts_40bit(frame_offset1.Data.SYNC.Ts));
      Handle_Com.GiveBuff((uint8_t *)&buffer, len);
      // Serial.printf("[DEBUG] Offset Frame (%d bytes): %s", len,
      //               buffer); // parse_ts_40bit(frame_offset1.Data.SYNC.Ts)
      //                        //(uint16_t)0
      return;
    }
  }
  //== >> tự xoạn protocol cho việt offset này.hoặc thêm vào cuối gói tag
}

/*TWR*/
// void DwHandle::twr(void)
// {
// 	if (this->mode != mode_twr && this->toway.flag == false)
// 		return;

// 	static uint64_t poll_rx;
// 	uint32_t tx_time;

// 	int8_t dw_read_flag;
// 	uint8_t buff_tx[SIZE_OF_DATAFRAME + 8];
// 	uint8_t buff_tx_aes[SIZE_OF_DATAFRAME + 8];

// 	static bool waiting_final = false;
// 	static unsigned long timeout_start = 0;

// 	// Nếu là lần đầu tiên gửi TWR
// 	if (this->toway.flag == true)
// 	{
// 		// this->set_recheck();
// 		//timeout_start = millis(); // Bắt đầu đếm thời gian timeout
// 		this->toway.flag = false;
// 		this->toway.data.Cmd = Cmd_start_twr;
// 		delay(10);
// 		send_ds_twr_command(this->toway.data.Des); // Gửi lệnh TWR
// 		//waiting_final = true;					   //
// Đánh dấu đang chờ phản hồi 		Serial.println("⏳ Đã gửi lệnh TWR, chờ
// phản hồi...");
// 	}

// 	this->mode = mode_wait;

// 	// // Kiểm tra timeout nếu chưa có phản hồi
// 	// if (waiting_final && (millis() - timeout_start >= 1000))
// 	// {
// 	// 	this->set_recheck();
// 	// 	Serial.println("⚠️ Quá thời gian chờ phản hồi TWR, gửi lại
// lệnh...");
// 	// 	reset_isp3080();
// 	// 	delay(10);
// 	// 	begin_dw3000();
// 	// 	delay(10);
// 	// 	set_device_id_to_nrf(parametter_dw.serial_id.device); // Gửi ID
// thiết bị đến nRF
// 	// 	delay(10);
// 	// 	send_ds_twr_command(this->toway.data.Des); // Gửi lại lệnh
// 	// 	timeout_start = millis();				   //
// Reset lại thời gian timeout
// 	// }

// 	// Nếu đã nhận được phản hồi từ final_twr
// 	if (final_twr)
// 	{
// 		uint32_t dist = frame_twr.Data.DIST.Dis;
// 		float dist_m = dist / 1000.0f;

// 		Serial.printf("📤 Distance: %.3f mét (từ %lu mm)\n", dist_m,
// dist);

// 		if (this->toway_callback != nullptr)
// 		{
// 			this->toway_callback(dist_m);
// 			Serial.println("✅ Đã gọi toway_callback thành công.");
// 		}
// 		else
// 		{
// 			Serial.println("⚠️ Không có callback toway_callback nào
// được gán.");
// 		}

// 		final_twr = false;
// 		waiting_final = false; // Reset cờ timeout sau khi đã xử lý xong
// 	}
// }

void DwHandle::twr(void)
{
  if (this->decawace_ready_f == false)
    return;

  if (this->mode != mode_twr && this->toway.flag == false)
    return;

  // dữ liệu timestamp dành cho toway
  static uint64_t poll_rx;
  uint32_t tx_time;

  /*dành cho dữ liệu truyền nhận*/
  int8_t dw_read_flag;
  uint8_t buff_tx[SIZE_OF_DATAFRAME + 8];
  uint8_t buff_tx_aes[SIZE_OF_DATAFRAME + 8];

  // neu gap lenh do toway dau tien
  if (this->toway.flag == true)
  {
    // dbg_dw_twr("first time");
    this->toway.flag = false;
    this->toway.data.Cmd = Cmd_start_twr;
    send_ds_twr_command(this->toway.data.Des);
    this->set_recheck_twr();
  }
  this->mode = mode_wait;

  if (final_twr == true) // hoặc chỉ cần: if (final_twr)
  {
    uint32_t dist = frame_twr.Data.DIST.Dis;
    float dist_m = dist / 1000.0f;

    // Serial.printf("📤 Distance: %.3f mét (từ %lu mm)\n", dist_m, dist);

    if (this->toway_callback != nullptr)
    {
      this->toway_callback(dist_m);
      // Serial.println("✅ Đã gọi toway_callback thành công.");
    }
    else
    {
      // Serial.println("⚠️ Không có callback toway_callback nào được gán.");
    }
    final_twr = false;
  }

  // switch (this->toway.data.Cmd)
  // {
  // case Cmd_start_twr:
  // {
  // 	// dbg_dw_twr("start");

  // 	dwt_forcetrxoff();

  // 	// Write frame data
  // 	this->toway.data.Cmd = Cmd_Pool;
  // 	this->toway.data.TypeDev = 0xBA;
  // 	this->toway.data.Packet_Id++;

  // 	memset(buff_tx, 0, sizeof(buff_tx));
  // 	memcpy(buff_tx, (uint8_t *)&this->toway.data, SIZE_OF_DATAFRAME);

  // 	// ma hoa du lieu
  // 	HW_AES_CBC128_2B_encryption(buff_tx_aes, buff_tx);

  // 	dwt_writetxdata(SIZE_OF_DATAFRAME + 2, buff_tx_aes, 0);
  // 	dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2, 0, 1);

  // 	/* If returns an error, abandon this ranging exchange and proceed to the
  // next one. */ 	if (dwt_starttx(DWT_START_TX_IMMEDIATE) == DWT_ERROR)
  // 	{
  // 		// dbg_dw_twr("Tx pool err");
  // 		// led_status.decawace(led_error_DW);
  // 		break;
  // 	}
  // 	// dbg_dw_twr("Tx pool ok");

  // 	break;
  // }
  // case Cmd_Pool:
  // {
  // 	poll_rx = data_rx_raw.rx_timestamp;
  // 	// dbg_dw_twr("PollRx ok !");

  // 	dwt_forcetrxoff();

  // 	tx_time = parametter_dw.anten_delay.tx;
  // 	tx_time = (data_rx_raw.rx_timestamp + (tx_time * UUS_TO_DWT_TIME)) >> 8;
  // 	dwt_setdelayedtrxtime(tx_time);

  // 	this->toway.data.Des = this->toway.data.Src;
  // 	this->toway.data.Src = parametter_dw.serial_id.device;
  // 	this->toway.data.Cmd = Cmd_Resp;

  // 	memset(buff_tx, 0, sizeof(buff_tx));
  // 	memcpy(buff_tx, (uint8_t *)&this->toway.data, SIZE_OF_DATAFRAME);

  // 	/* ma hoa du lieu */
  // 	HW_AES_CBC128_2B_encryption(buff_tx_aes, buff_tx);

  // 	dwt_writetxdata(SIZE_OF_DATAFRAME + 2, buff_tx_aes, 0); /* Zero offset
  // in TX buffer. */ 	dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2, 0, 1);
  // /* Zero offset in TX buffer, ranging. */

  // 	// If returns an error, abandon this ranging exchange and proceed to the
  // next one. 	if (dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) ==
  // DWT_ERROR)
  // 	{
  // 		// dbg_dw_twr("Tx Resp error");
  // 		// led_status.decawace(led_error_DW);
  // 		break;
  // 	}
  // 	// dbg_dw_twr("Tx Resp ok");

  // 	break;
  // }
  // case Cmd_Resp: // trả lời lệnh đo twr
  // {
  // 	// dbg_dw_twr("RespRx");

  // 	dwt_forcetrxoff();

  // 	tx_time = parametter_dw.anten_delay.tx;
  // 	tx_time = (data_rx_raw.rx_timestamp + (tx_time * UUS_TO_DWT_TIME)) >> 8;
  // 	dwt_setdelayedtrxtime(tx_time);

  // 	// Final TX timestamp is the transmission time we programmed plus the TX
  // antenna delay. 	uint64_t FinalTxTs; 	FinalTxTs = (((uint64_t)(tx_time
  // & 0xFFFFFFFEUL)) << 8) + parametter_dw.anten_delay.tx;

  // 	this->toway.data.Des = this->toway.data.Src;
  // 	this->toway.data.Src = parametter_dw.serial_id.device;
  // 	this->toway.data.Cmd = Cmd_Final;
  // 	tdoa_set_timestamp_u64(data_rx_raw.tx_timestamp,
  // this->toway.data.Data.DS_TWR.TIME_STAMP.poll_tx);
  // 	tdoa_set_timestamp_u64(data_rx_raw.rx_timestamp,
  // this->toway.data.Data.DS_TWR.TIME_STAMP.resp_rx);
  // 	tdoa_set_timestamp_u64(FinalTxTs,
  // this->toway.data.Data.DS_TWR.TIME_STAMP.final_tx);

  // 	memset(buff_tx, 0, sizeof(buff_tx));
  // 	memcpy(buff_tx, (uint8_t *)&this->toway.data, SIZE_OF_DATAFRAME);

  // 	/* ma hoa du lieu */
  // 	HW_AES_CBC128_2B_encryption(buff_tx_aes, buff_tx);

  // 	dwt_writetxdata(SIZE_OF_DATAFRAME + 2, buff_tx_aes, 0); /* Zero offset
  // in TX buffer. */ 	dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2, 0, 1);
  // /* Zero offset in TX buffer, ranging. */

  // 	// If returns an error, abandon this ranging exchange and proceed to the
  // next one. 	if (dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) ==
  // DWT_ERROR)
  // 	{
  // 		// dbg_dw_twr("Tx Final error");
  // 		// led_status.decawace(led_error_DW);
  // 		break;
  // 	}
  // 	// dbg_dw_twr("Tx Final ok");

  // 	break;
  // }
  // case Cmd_Final: // tính được khoảng cách
  // {
  // 	dwt_forcetrxoff();

  // 	tx_time = parametter_dw.anten_delay.tx;
  // 	tx_time = (data_rx_raw.rx_timestamp + (tx_time * UUS_TO_DWT_TIME)) >> 8;
  // 	dwt_setdelayedtrxtime(tx_time);

  // 	// Bien cho tinh khoang cach
  // 	uint64_t poll_tx_ts, resp_rx_ts, final_tx_ts;
  // 	double Ra, Rb, Da, Db;
  // 	int64_t tof_dtu;

  // 	poll_tx_ts =
  // tdoa_get_timestamp_u64(this->toway.data.Data.DS_TWR.TIME_STAMP.poll_tx);
  // 	resp_rx_ts =
  // tdoa_get_timestamp_u64(this->toway.data.Data.DS_TWR.TIME_STAMP.resp_rx);
  // 	final_tx_ts =
  // tdoa_get_timestamp_u64(this->toway.data.Data.DS_TWR.TIME_STAMP.final_tx);

  // 	/* Compute time of flight. 32-bit subtractions give correct answers even
  // if clock has wrapped. */ 	Ra = (double)((resp_rx_ts - poll_tx_ts) &
  // 0xFFFFFFFFUL); 	Rb = (double)((data_rx_raw.rx_timestamp -
  // data_rx_raw.tx_timestamp) & 0xFFFFFFFFUL); 	Da =
  // (double)((final_tx_ts - resp_rx_ts) & 0xFFFFFFFFUL); 	Db =
  // (double)((data_rx_raw.tx_timestamp - poll_rx) & 0xFFFFFFFFUL);
  // 	// dbg_dw_twr("Ra:%10.5f | Rb:%10.5f | Da:%10.5f | Db:%10.5f", Ra, Rb,
  // Da, Db); 	tof_dtu = (int64_t)((Ra * Rb - Da * Db) / (Ra + Rb + Da + Db));

  // 	if (tof_dtu < 0)
  // 	{
  // 		// dbg_dw_twr("Tof_dtu err");
  // 		tof_dtu = 0;
  // 	}
  // 	/* Write and send the response message. */
  // 	this->toway.data.Des = this->toway.data.Src;
  // 	this->toway.data.Src = parametter_dw.serial_id.device;
  // 	this->toway.data.Cmd = Cmd_Distance;
  // 	this->toway.data.Data.DIST.Dis = (int32_t)tof_dtu;

  // 	memset(buff_tx, 0, sizeof(buff_tx));
  // 	memcpy(buff_tx, (uint8_t *)&this->toway.data, SIZE_OF_DATAFRAME);

  // 	/* ma hoa du lieu */
  // 	HW_AES_CBC128_2B_encryption(buff_tx_aes, buff_tx);

  // 	dwt_writetxdata(SIZE_OF_DATAFRAME + 2, buff_tx_aes, 0); /* Zero offset
  // in TX buffer. */ 	dwt_writetxfctrl(SIZE_OF_DATAFRAME + 2, 0, 1);
  // /* Zero offset in TX buffer, ranging. */

  // 	/* If returns an error, abandon this ranging exchange and proceed to the
  // next one.*/ 	if (dwt_starttx(DWT_START_TX_DELAYED |
  // DWT_RESPONSE_EXPECTED)
  // == DWT_ERROR)
  // 	{
  // 		// led_status.decawace(led_error_DW);
  // 		// dbg_dw_twr("EndTx Error");
  // 		break;
  // 	}

  // 	double Dist, tf;
  // 	tf = tof_dtu * DWT_TIME_UNITS;
  // 	Dist = tf * SPEED_OF_LIGHT;
  // 	// dbg_dw_twr("[%u-%u] Counter: %lld", this->toway.data.Src,
  // this->toway.data.Des, tof_dtu);
  // 	// dbg_dw_twr("Dist: %3.2f m", Dist);
  // 	break;
  // }
  // case Cmd_Distance: // gửi khoảng cách cho đối diện
  // {
  // 	// dbg_dw_twr("Distance ok");

  // 	double Dist, tf;
  // 	tf = this->toway.data.Data.DIST.Dis * DWT_TIME_UNITS;
  // 	Dist = tf * SPEED_OF_LIGHT;
  // 	// dbg_dw_twr("[%u-%u] Counter: %ld", this->toway.data.Src,
  // this->toway.data.Des, this->toway.data.Data.DIST.Dis); dbg_dw_twr("Dist:
  // %3.2f m", Dist);

  // 	// report lại kích thước
  // 	if (this->toway_callback != NULL)
  // 		this->toway_callback(Dist);

  // 	break;
  // }
  // }

  // chuyển sang mode nhận sau khi trans
  // this->Buffer_En();
  // this->mode = mode_wait;
}

/*Re Check decawave*/
void DwHandle::set_recheck(void) { this->decawave_recheck_to->ToEUpdate(5000); }

void DwHandle::set_recheck_twr(void)
{
  this->decawave_recheck_to->ToEUpdate(20000);
}

void DwHandle::recheck(void)
{
  if (this->decawave_recheck_to->ToEExpired())
  {
    // 1. Chặn reset khi đang trong quá trình cấu hình (is_sending_config ==
    // true) Hoặc vừa mới gửi cấu hình xong (last_quiet_ms đang ở tương lai)
    if (is_sending_config || (millis() < last_quiet_ms))
    {
      this->set_recheck(); // Đẩy lùi thêm 5s nữa
      return;
    }

    // 2. Chặn reset khi ở mode TWR DucThang (Mode 5)
    if (temp_id_mode == 5)
    {
      this->set_recheck();
      return;
    }

    // 3. Thực hiện Reset ISP + DW3000 nếu im lặng thật sự > 5s
    this->begin();
  }
}

/*calculator timestamp*/
uint64_t DwHandle::tdoa_get_timestamp_u64(uint8_t *Dat)
{
  uint64_t ts = 0;
  for (int i = 4; i >= 0; i--)
  {
    ts <<= 8;
    ts |= Dat[i];
  }
  return ts;
}

void DwHandle::tdoa_set_timestamp_u64(uint64_t Ts, uint8_t *Dat)
{
  for (int i = 0; i <= 4; i++)
  {
    Dat[i] = Ts;
    Ts >>= 8;
  }
}

uint64_t DwHandle::get_tx_timestamp_u64(void)
{
  uint8_t ts_tab[5];
  uint64_t ts = 0;
  dwt_readtxtimestamp(ts_tab);
  for (int i = 4; i >= 0; i--)
  {
    ts <<= 8;
    ts |= ts_tab[i];
  }
  return ts;
}

uint64_t DwHandle::get_rx_timestamp_u64(void)
{
  uint8_t ts_tab[5];
  uint64_t ts = 0;
  dwt_readrxtimestamp(ts_tab);
  for (int i = 4; i >= 0; i--)
  {
    ts <<= 8;
    ts |= ts_tab[i];
  }
  return ts;
}

uint8_t DwHandle::value2array(uint8_t *Array, uint64_t Value, uint8_t Length)
{
  for (uint8_t i = 0; i < Length; i++)
  {
    Array[i] = (Value >> (i * 8)) & 0xFF;
  }
  return Length;
}
DwHandle Handle_Dw;
