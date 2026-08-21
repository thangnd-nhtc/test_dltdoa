#include "handle_spi_master.h"
// #include <ESP32DMASPIMaster.h>
#include "handle_com_regs.h"
#include "handle_config.h"
#include "handle_ethernet.h"
#include "handle_mqtt.h"
#include "handle_tcp.h"
#include <ESP32DMASPISlave.h>
#include <TimeOutEvent.h>

#include "button_control.h"

#include "OTA.h"
// #define CMD_SET_BEACON_CFG  0xB1

// ESP32DMASPI::Master master;
ESP32DMASPI::Slave slave;

TimeOutEvent SPI_TimeCheck(2000);
uint8_t *spi_slave_rx_buf;
// uint8_t *spi_master_tx_buf;
#define BuffDMA_Size (SIZE_TCP_BUFFER * 300)
static uint8_t *BUFFER_DMA;
volatile uint32_t Position_Buff_Wr = 0;
volatile uint32_t Position_Buff_Rd = 0;
bool FagOverfull = false;
uint16_t FlagRead = 0;
bool FlagcheckLed = false;
// volatile bool flag = false;
volatile bool flagsend = true;
bool OTAstart = false;

TaskHandle_t task_reviver;

handle_spi::handle_spi(/* args */)
{
  BUFFER_DMA = slave.allocDMABuffer(BuffDMA_Size);
  // BUFFER_DMA = (uint8_t *)malloc(BuffDMA_Size);
  spi_slave_rx_buf = slave.allocDMABuffer(BUFFER_SIZE);
  // spi_master_tx_buf = master.allocDMABuffer(BUFFER_SIZE);
}

handle_spi::~handle_spi() {}

void loop_reciver(void *arg)
{
  while (1)
  {
    if (slave.remained() == 0)
    {
      // memset(spi_slave_rx_buf,0,BUFFER_SIZE);
      slave.wait(spi_slave_rx_buf, SIZE_TCP_BUFFER);
    }
    // slave.yield();
    // delay(1);
  }
}

void handle_spi::begin()
{

  slave.setDataMode(SPI_MODE3);
  // slave.setMaxTransferSize(SPI_SALVE_BUFF_SIZE_TX);
  slave.setDMAChannel(2); // 1 or 2 only
  slave.setQueueSize(1);  // transaction queue size

  slave.setTimeout(10);
  slave.begin(HSPI, SPI_COM_CLK, SPI_COM_MISO, SPI_COM_MOSI, SPI_COM_CS);
  // xTaskCreatePinnedToCore(loop_reciver, "spi_loop_reciver", 2049, NULL, 2,
  // &task_reviver, 0);
  /*
      master.setDataMode(SPI_MODE0);
      // master.setFrequency(SPI_MASTER_FREQ_8M); // too fast for bread board...
      master.setFrequency(4000000);
      master.setMaxTransferSize(BUFFER_SIZE);
      // master.setDMAChannel(2); // 1 or 2 only
      // master.setQueueSize(1);  // transaction queue size
      // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12
      // master.begin();  // default SPI is HSPI
      // master.begin(HSPI,14,12,13,4);  // default SPI is HSPI
      master.begin(HSPI, SPI_COM_CLK, SPI_COM_MISO, SPI_COM_MOSI, SPI_COM_CS);
     // default SPI is HSPI
      */
}

void handle_spi::reset()
{
  //    master.end();
  //    this->begin();
}

void handle_spi::masterTransfer(uint8_t *bufferSend, uint32_t len)
{
  // memset(spi_master_tx_buf, 0, BUFFER_SIZE);
  // memcpy(spi_master_tx_buf, bufferSend, len);
  // Serial.printf("\r\n send:");
  // for (unsigned int i = 0; i < len; i++)
  // 	Serial.printf("%d ", spi_master_tx_buf[i]);
  // Serial.println("");
  // master.transfer(spi_master_tx_buf, spi_master_rx_buf, len + 3);
  // memset(spi_master_rx_buf, 0, BUFFER_SIZE);
}

// void handle_spi::masterRead(uint8_t *bufferread)
// {
//     memset((uint8_t *)spi_master_tx_buf, 0, BUFFER_SIZE);
//     memset((uint8_t *)spi_master_rx_buf, 0, SIZE_TCP_BUFFER);
//     master.transfer(spi_master_tx_buf, spi_master_rx_buf, SIZE_TCP_BUFFER);
//     // Serial.printf("\r\n nhan 200:");
//     // for (unsigned int i = 0; i < SIZE_TCP_BUFFER; i++)
//     // 	Serial.printf("%d ", spi_master_rx_buf[i]);
//     // Serial.println("");

//     memcpy((uint8_t *)bufferread, spi_master_rx_buf, SIZE_TCP_BUFFER);
//     // master.yield();
//     memset(spi_master_rx_buf, 0, SIZE_TCP_BUFFER);
// }

// uint16_t handle_spi::masterGet(uint8_t* address, uint8_t* bufferRead,
// uint32_t timeout)
// {
//     SPI_TimeCheck.ToEUpdate(timeout);
//     int i = 0;
//     memset(spi_master_rx_buf,NULL,BUFFER_SIZE);
//     master.transfer(address, spi_master_rx_buf, BUFFER_SIZE);
//      while(SPI_TimeCheck.ToEExpired() == false)
//      {
//           master.yield();
//          if(spi_master_rx_buf[i] == '\n' && i <= BUFFER_SIZE)
//          {
//             i++;
//             memcpy(bufferRead, spi_master_rx_buf, i);
//             memset(spi_master_rx_buf,0,BUFFER_SIZE);
//             return i;
//          }
//        i++;
//      }
//     SPI_TimeCheck.ToEDisable();
//     return 0;

// }

uint16_t handle_spi::calcCRC(uint8_t *data, size_t size)
{
  if (size <= BUFFER_SIZE)
  {
    uint16_t CrcPoly_U16 = 0x8408;
    uint16_t Crc_U16 = 0;
    uint8_t j, i_bits, Carry_U8;

    for (j = 0; j < size; j++)
    {
      Crc_U16 = Crc_U16 ^ data[j];

      for (i_bits = 0; i_bits < 8; i_bits++)
      {
        Carry_U8 = Crc_U16 & 1;
        Crc_U16 = Crc_U16 / 2;

        if (Carry_U8)
          Crc_U16 = Crc_U16 ^ CrcPoly_U16;
      }
    }

    return Crc_U16;
  }
  else
  {
    // Serial.println("Data lenght over long");
  }

  return 0xffff;
}

// void addBuff(char *Buf, uint16_t Len)
// {
// static uint32_t OverNum = 0, OverNum_old = 0;
// if (Len > BuffPacket_Size)
// {
// 	debug_TCP("Packet over size");
// 	return;
// }
// if (++BuffShare_Wr_Id >= BuffPacket_Num)
// 	BuffShare_Wr_Id = 0;

// memset((uint8_t *)BuffShare[BuffShare_Wr_Id], 0, BuffPacket_Size);
// memcpy((uint8_t *)BuffShare[BuffShare_Wr_Id], (uint8_t*)Buf, Len);
// BuffSize[BuffShare_Wr_Id] = Len;

// Tdoa_total_packet += 1;
// /*Check over load data temp*/
// if (BuffShare_Wr_Id == BuffShare_Rd_Id)
// {
// 	if (++BuffShare_Rd_Id >= BuffPacket_Num)
// 		BuffShare_Rd_Id = 0;
// 	OverNum++;
// }
// else if (OverNum_old != OverNum)
// {
// 	Tdoa_lost_packet += OverNum - OverNum_old;
// 	debug_TCP("Overload %lu packet Tdoa. %llu/%llu", OverNum - OverNum_old,
// Tdoa_lost_packet, Tdoa_total_packet);
// 	// debug_TCP("Tdoa overload buffshare %lu packet", OverNum -
// OverNum_old); 	OverNum_old = OverNum;
// }
// }

void handle_spi::addBufferDMA(uint8_t *Buf, uint16_t Length)
{
  for (uint16_t i = 0; i < Length; i++)
  {
    BUFFER_DMA[Position_Buff_Wr] = Buf[i];
    Position_Buff_Wr++;
    if (Position_Buff_Wr > BuffDMA_Size)
    {
      Position_Buff_Wr = 0;
      // FagOverfull = true;
      debug_TCP("bufffer DMA full");
    }
  }
  // Position_Buff_Wr--;
}

uint32_t handle_spi::Buff_is_available(void)
{
  uint32_t Num = 0;
  if (Position_Buff_Wr > Position_Buff_Rd)
    Num = Position_Buff_Wr - Position_Buff_Rd;
  else if (Position_Buff_Wr < Position_Buff_Rd)
  {
    Num = (BuffDMA_Size - Position_Buff_Rd) + Position_Buff_Wr;
    FagOverfull = false;
  }

  return Num;
}

uint8_t handle_spi::readBufferDMA(uint8_t *data, uint32_t Length)
{
  if (!Length)
    return 0;

  memset(data, NULL, Length);

  for (uint32_t i = 0; i < Length; i++)
  {
    data[i] = BUFFER_DMA[Position_Buff_Rd];
    Position_Buff_Rd++;
    if (Position_Buff_Rd > BuffDMA_Size)
      Position_Buff_Rd = 0;
  }
  return 1;
}

void handle_spi::reciver_callback(HandlerFunction handle)
{
  this->rx_callback = handle;
}
char data_DW[SIZE_TCP_BUFFER];
uint16_t length_data_DW = 0;
void handle_spi::loop(void)
{
  if (slave.remained() == 0)
  {
    slave.queue(spi_slave_rx_buf, SIZE_TCP_BUFFER);
  }

  // if transaction has completed from master,
  // available() returns size of results of transaction,
  // and buffer is automatically updated

  while (slave.available())
  {
    // show received data
    memset((char *)data_DW, NULL, sizeof(data_DW));
    for (size_t i = 0; i < slave.size(); ++i)
    {

      data_DW[i] = spi_slave_rx_buf[i];
      if (data_DW[i] == 58)
      {
        i++;
        data_DW[i] = '\n';
        i++;
        data_DW[i] = '\r';
        length_data_DW = (uint16_t)i;
        // debug_SPI("so len = %d\n\r",length_data_DW);
        break;
      }
    }
    slave.pop();
    this->rx_callback((uint8_t *)data_DW, length_data_DW);
  }

  //     uint8_t i = 0;
  // if(slave.available())
  // {
  //     memcpy(data_DW,(char*)spi_slave_rx_buf,SIZE_TCP_BUFFER);
  //     slave.pop();
  //     memset(spi_slave_rx_buf,NULL,SIZE_TCP_BUFFER);

  //     this->rx_callback((uint8_t*)data_DW, strlen(data_DW));

  //     // for(int)

  //     // if (length_rx > 0 && length_rx < SIZE_TCP_BUFFER &&
  //     this->rx_callback != NULL)
  //     // {
  //     //     this->rx_callback(spi_slave_rx_buf, length_rx);

  //     //     // this->clearDMA();
  //     //     memset(spi_slave_rx_buf,0,BUFFER_SIZE);
  //     //     slave.pop();
  //     //     return;
  //     // }
  // }
  // }
}

void sendUART(uint8_t *bufferSend, uint32_t len)
{
  for (int i = 0; i < len; i++)
    Serial1.write(bufferSend[i]);
}

void handle_spi::setConfigDW(void)
{
  // ==== CMD 8 (DS-TWR): Xử lý ĐỘC LẬP, KHÔNG bị block bởi LED/config ====
  if (Fag_mask_regs.FAG_TWO_WAY_RES == 2)
  {
    memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = read_distant;
    this->tx_frame.header.msk_regs = SET_TWO_WAY;
    two_way.deviceID = MQTT_Exchange.cmd8_9_10_11.BaseID;
    two_way.distance = 0;
    memcpy((uint8_t *)this->tx_frame.data, &two_way, sizeof(two_way));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);
    Serial1.write((uint8_t *)&this->tx_frame,
                  sizeof(this->tx_frame.header) +
                      this->tx_frame.header.msk_regs.len);
    debug_SPI("set FAG_TWO_WAY_RES");
    Fag_mask_regs.FAG_TWO_WAY_RES = 1;
  }

  if (FlagcheckLed)
  {
    this->check_status_conncet();
    this->read_led();
    FlagcheckLed = false;
  }

  else if (Fag_mask_regs.FAG_SerialID_RES == 2)
  {
    memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = write_config;
    this->tx_frame.header.msk_regs = SerialID_RES;
    SerialID.device = Config_Device.Device.SerialID;
    SerialID.broadcast = 0;
    memcpy((uint8_t *)this->tx_frame.data, &SerialID, sizeof(SerialID));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);
    Serial1.write((uint8_t *)&this->tx_frame,
                  sizeof(this->tx_frame.header) +
                      this->tx_frame.header.msk_regs.len);
    debug_SPI("set config FAG_SerialID_RES");
    Fag_mask_regs.FAG_SerialID_RES = 1;
  }

  else if (Fag_mask_regs.FAG_MASTER_ACCESS_RES1 == 2)
  {
    memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = write_config;
    this->tx_frame.header.msk_regs = MASTER_ACCESS_RES1;
    memcpy((uint8_t *)this->tx_frame.data, &master_access,
           sizeof(master_access));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);

    Serial1.write((uint8_t *)&this->tx_frame,
                  sizeof(this->tx_frame.header) +
                      this->tx_frame.header.msk_regs.len);
    debug_SPI("set config FAG_MASTER_ACCESS_RES1");
    Fag_mask_regs.FAG_MASTER_ACCESS_RES1 = 1;
  }

  else if (Fag_mask_regs.FAG_MASTER_ACCESS_RES2 == 2)
  {
    memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = write_config;
    this->tx_frame.header.msk_regs = MASTER_ACCESS_RES2;
    memcpy((uint8_t *)this->tx_frame.data, &master_access,
           sizeof(master_access));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);

    Serial1.write((uint8_t *)&this->tx_frame,
                  sizeof(this->tx_frame.header) +
                      this->tx_frame.header.msk_regs.len);
    debug_SPI("set config FAG_MASTER_ACCESS_RES2");
  }

  else if (Fag_mask_regs.FAG_MASTER_ACCESS_RES3 == 2)
  {
    memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = write_config;
    this->tx_frame.header.msk_regs = MASTER_ACCESS_RES3;
    memcpy((uint8_t *)this->tx_frame.data, &master_access,
           sizeof(master_access));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);

    Serial1.write((uint8_t *)&this->tx_frame,
                  sizeof(this->tx_frame.header) +
                      this->tx_frame.header.msk_regs.len);
    debug_SPI("set config FAG_MASTER_ACCESS_RES3");
    Fag_mask_regs.FAG_MASTER_ACCESS_RES3 = 1;
  }

  else if (Fag_mask_regs.FAG_MASTER_ACCESS_RES4 == 2)
  {
    memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = write_config;
    this->tx_frame.header.msk_regs = MASTER_ACCESS_RES4;
    memcpy((uint8_t *)this->tx_frame.data, &master_access,
           sizeof(master_access));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);

    Serial1.write((uint8_t *)&this->tx_frame,
                  sizeof(this->tx_frame.header) +
                      this->tx_frame.header.msk_regs.len);
    debug_SPI("set config FAG_MASTER_ACCESS_RES4");
    Fag_mask_regs.FAG_MASTER_ACCESS_RES4 = 1;
  }

  else if (Fag_mask_regs.FAG_DW_CONFIG_RES == 2)
  {
    memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = write_config;
    this->tx_frame.header.msk_regs = DW_CONFIG_RES;
    memcpy((uint8_t *)this->tx_frame.data, &dw_config_tdoa,
           sizeof(dw_config_tdoa));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);
    Serial1.write((uint8_t *)&this->tx_frame,
                  sizeof(this->tx_frame.header) +
                      this->tx_frame.header.msk_regs.len);
    debug_SPI("set config FLAG_DW_CONFIG_RES");
    Fag_mask_regs.FAG_DW_CONFIG_RES = 1;
    delay(500);
    SPI_TimeCheck.ToEUpdate(500);
  }

  else if (Fag_mask_regs.FAG_DW_CONFIG_TX_RES == 2)
  {
    memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = write_config;
    this->tx_frame.header.msk_regs = DW_CONFIG_TX_RES;
    memcpy((uint8_t *)this->tx_frame.data, &dw_txconfig_tdoa,
           sizeof(dw_txconfig_tdoa));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);
    Serial1.write((uint8_t *)&this->tx_frame,
                  sizeof(this->tx_frame.header) +
                      this->tx_frame.header.msk_regs.len);
    debug_SPI("set config FLAG_DW_CONFIG_TX_RES");
    Fag_mask_regs.FAG_DW_CONFIG_TX_RES = 1;
    delay(500);
    SPI_TimeCheck.ToEUpdate(500);
  }

  else if (Fag_mask_regs.FAG_DW_ANT_DELAY_RES == 2)
  {
    memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = write_config;
    this->tx_frame.header.msk_regs = DW_ANT_DELAY_RES;
    memcpy((uint8_t *)this->tx_frame.data, &anten_delay_tdoa,
           sizeof(anten_delay_tdoa));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);
    Serial1.write((uint8_t *)&this->tx_frame,
                  sizeof(this->tx_frame.header) +
                      this->tx_frame.header.msk_regs.len);
    debug_SPI("set config FLAG_DW_ANT_DELAY_RES");
    Fag_mask_regs.FAG_DW_ANT_DELAY_RES = 1;
    delay(500);
    SPI_TimeCheck.ToEUpdate(500);
  }

  else if (Fag_mask_regs.FAG_MASTER_RES == 2)
  {
    memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = write_config;
    this->tx_frame.header.msk_regs = MASTER_RES;
    memcpy((uint8_t *)this->tx_frame.data, &dw_master_tdoa,
           sizeof(dw_master_tdoa));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);

    Serial1.write((uint8_t *)&this->tx_frame,
                  sizeof(this->tx_frame.header) +
                      this->tx_frame.header.msk_regs.len);
    debug_SPI("set config FLAG_MASTER_RES");
    Fag_mask_regs.FAG_MASTER_RES = 1;
    delay(500);

    SPI_TimeCheck.ToEUpdate(500);
  }
  // else if (Fag_mask_regs.FAG_BEACON_RES == 2)
  // {
  //     memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
  //     this->tx_frame.header.type = write_config;
  //     this->tx_frame.header.msk_regs = BEACON_CFG_RES;
  //     memcpy((uint8_t *)this->tx_frame.data, &g_beacon_cfg,
  //     sizeof(g_beacon_cfg)); this->tx_frame.header.check_crc =
  //     SPI_master.calcCRC(this->tx_frame.data,
  //     this->tx_frame.header.msk_regs.len); Serial1.write((uint8_t
  //     *)&this->tx_frame,sizeof(this->tx_frame.header) +
  //     this->tx_frame.header.msk_regs.len); debug_SPI("send BEACON_CFG");
  //     Fag_mask_regs.FAG_BEACON_RES = 1;
  // }
}

void handle_spi::readConfigDW(void)
{
  if (SPI_TimeCheck.ToEExpired())
  {
    SPI_TimeCheck.ToEUpdate(300);
    if (Fag_mask_regs.FAG_DW_CONFIG_RES == 1)
    {
      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = read_config;
      tx_frame.header.msk_regs = DW_CONFIG_RES;
      tx_frame.header.check_crc =
          SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
      Serial1.write((uint8_t *)&tx_frame,
                    sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
      debug_SPI("read config FLAG_DW_CONFIG_RES");
    }

    else if (Fag_mask_regs.FAG_SerialID_RES == 1)
    {
      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = read_config;
      tx_frame.header.msk_regs = SerialID_RES;
      tx_frame.header.check_crc =
          SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
      Serial1.write((uint8_t *)&tx_frame,
                    sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
      debug_SPI("read config FAG_SerialID_RES");
    }
    else if (Fag_mask_regs.FAG_DW_CONFIG_TX_RES == 1)
    {

      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = read_config;
      tx_frame.header.msk_regs = DW_CONFIG_TX_RES;
      tx_frame.header.check_crc =
          SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
      Serial1.write((uint8_t *)&tx_frame,
                    sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
      debug_SPI("read config FLAG_DW_CONFIG_TX_RES");
    }
    else if (Fag_mask_regs.FAG_DW_ANT_DELAY_RES == 1)
    {
      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = read_config;
      tx_frame.header.msk_regs = DW_ANT_DELAY_RES;
      tx_frame.header.check_crc =
          SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
      Serial1.write((uint8_t *)&tx_frame,
                    sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
      debug_SPI("read config FLAG_DW_ANT_DELAY_RES");
    }
    else if (Fag_mask_regs.FAG_MASTER_RES == 1)
    {
      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = read_config;
      tx_frame.header.msk_regs = MASTER_RES;
      tx_frame.header.check_crc =
          SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
      Serial1.write((uint8_t *)&tx_frame,
                    sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
      debug_SPI("read config FLAG_MASTER_RES");
    }

    else if (Fag_mask_regs.FAG_MASTER_ACCESS_RES1 == 1)
    {

      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = read_config;
      tx_frame.header.msk_regs = MASTER_ACCESS_RES1;
      tx_frame.header.check_crc =
          SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
      Serial1.write((uint8_t *)&tx_frame,
                    sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
      debug_SPI("read config MASTER_ACCESS_RES1");
    }

    else if (Fag_mask_regs.FAG_MASTER_ACCESS_RES2 == 1)
    {
      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = read_config;
      tx_frame.header.msk_regs = MASTER_ACCESS_RES2;
      tx_frame.header.check_crc =
          SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
      Serial1.write((uint8_t *)&tx_frame,
                    sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
      debug_SPI("read config MASTER_ACCESS_RES2");
    }

    else if (Fag_mask_regs.FAG_MASTER_ACCESS_RES3 == 1)
    {

      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = read_config;
      tx_frame.header.msk_regs = MASTER_ACCESS_RES3;
      tx_frame.header.check_crc =
          SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
      Serial1.write((uint8_t *)&tx_frame,
                    sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
      debug_SPI("read config MASTER_ACCESS_RES3");
    }

    else if (Fag_mask_regs.FAG_MASTER_ACCESS_RES4 == 1)
    {

      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = read_config;
      tx_frame.header.msk_regs = MASTER_ACCESS_RES4;
      tx_frame.header.check_crc =
          SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
      Serial1.write((uint8_t *)&tx_frame,
                    sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
      debug_SPI("read config MASTER_ACCESS_RES4");
    }

    else if (Fag_mask_regs.FAG_VERSION_RES == 1)
    {
      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = read_ram;
      tx_frame.header.msk_regs = READ_VERSION;
      tx_frame.header.check_crc =
          SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
      Serial1.write((uint8_t *)&tx_frame,
                    sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
      debug_SPI("read config FAG_VERSION_RES");
    }
  }
}

void handle_spi::readFragmentStatus()
{
  memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
  tx_frame.header.type = read_ram;
  tx_frame.header.msk_regs = FRAGMENT_STATUS_RES;
  tx_frame.header.check_crc =
      SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
  Serial1.write((uint8_t *)&tx_frame,
                sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
  debug_SPI("read fragment status");
}

void handle_spi::read_led()
{
  // if (Fag_mask_regs.FAG_HANDLE_LED_STATUS_RES == 1)
  // {
  // Fag_mask_regs.FAG_HANDLE_LED_STATUS_RES = 0;
  memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
  tx_frame.header.type = read_ram;
  tx_frame.header.msk_regs = HANDLE_LED_STATUS_RES;
  tx_frame.header.check_crc =
      SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
  Serial1.write((uint8_t *)&tx_frame,
                sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
  debug_SPI("read FAG_HANDLE_LED_STATUS_RES");
  // }
}

void handle_spi::readISPVersion()
{
  memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
  tx_frame.header.type = read_ram;
  tx_frame.header.msk_regs = ISP_VERSION_RES;
  tx_frame.header.check_crc =
      SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
  Serial1.write((uint8_t *)&tx_frame,
                sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
  debug_SPI("read ISP version");
}

void handle_spi::check_status_conncet()
{
  handle_led_status_t handle_led_status;
  const bool eth_status = eth_connected;
  const bool wifi_status = wifi_connected;
  const bool mqtt_status = Mqtt_Handle.mqtt_isconnect();
  const bool tcp1_status = _handle_tcp.Tdoa_Client1_Status();
  const bool tcp2_status = _handle_tcp.Tdoa_Client2_Status();

  memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
  this->tx_frame.header.type = write_ram;
  this->tx_frame.header.msk_regs = HANDLE_LED_STATUS_RES;

  if (!eth_status && !wifi_status)
  {
    handle_led_status.internet = (uint8_t)led_internet_fail;
  }

  else if ((eth_status || wifi_status) && mqtt_status &&
           (tcp1_status || tcp2_status))
  {
    handle_led_status.internet = (uint8_t)led_internet_ok;
  }

  else if ((eth_status || wifi_status) && !mqtt_status &&
           (!tcp1_status && !tcp2_status))
  {
    handle_led_status.internet = (uint8_t)led_fail_server;
  }

  else if ((eth_status || wifi_status) && tcp1_status && tcp2_status &&
           !mqtt_status)
  {
    handle_led_status.internet = (uint8_t)led_fail_MQTT;
  }

  else if ((eth_status || wifi_status) && mqtt_status &&
           (!tcp1_status && !tcp2_status))
  {
    handle_led_status.internet = (uint8_t)led_fail_TCP;
  }
  handle_led_status.power = 1;

  memcpy((uint8_t *)this->tx_frame.data, (uint8_t *)&handle_led_status,
         sizeof(handle_led_status));

  this->tx_frame.header.check_crc = SPI_master.calcCRC(
      this->tx_frame.data, this->tx_frame.header.msk_regs.len);
  debug_SPI("[LED-U6-TX] eth=%d wifi=%d mqtt=%d tcp1=%d tcp2=%d I=%d P=%d",
            eth_status, wifi_status, mqtt_status, tcp1_status, tcp2_status,
            handle_led_status.internet, handle_led_status.power);
  Serial1.write((uint8_t *)&this->tx_frame,
                sizeof(this->tx_frame.header) +
                    this->tx_frame.header.msk_regs.len);
  debug_SPI("set config FAG_HANDLE_LED_STATUS_RES \n\r");
}

// static uint16_t crc16_ibm(const uint8_t* data, size_t len) {
//   uint16_t crc = 0xFFFF;
//   for (size_t i = 0; i < len; ++i) {
//     crc ^= data[i];
//     for (int b = 0; b < 8; ++b)
//       crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
//   }
//   return crc;
// }

// void handle_spi::setConfigBeacon()
// {
//     if (Fag_mask_regs.FAG_BEACON_RES == 2)
//     {
//         memset(&this->tx_frame, 0, sizeof(this->tx_frame));
//         this->tx_frame.header.type = write_config;
//         this->tx_frame.header.msk_regs = BEACON_CFG_RES;
//         this->tx_frame.header.msk_regs.len = sizeof(beacon_cfg_t);

//         memcpy(this->tx_frame.data, &g_beacon_cfg, sizeof(g_beacon_cfg));
//         this->tx_frame.header.check_crc =
//             SPI_master.calcCRC(this->tx_frame.data,
//             this->tx_frame.header.msk_regs.len);

//         const size_t pkt_len = sizeof(this->tx_frame.header) +
//         this->tx_frame.header.msk_regs.len; Serial1.write((uint8_t
//         *)&this->tx_frame, pkt_len); debug_SPI("Duc Thang >> Sent BEACON_CFG
//         (len=%u)", (unsigned)this->tx_frame.header.msk_regs.len); debug_SPI("
//         active     = %u", g_beacon_cfg.active); debug_SPI("   uuid       =
//         %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
//                   g_beacon_cfg.uuid[0], g_beacon_cfg.uuid[1],
//                   g_beacon_cfg.uuid[2], g_beacon_cfg.uuid[3],
//                   g_beacon_cfg.uuid[4], g_beacon_cfg.uuid[5],
//                   g_beacon_cfg.uuid[6], g_beacon_cfg.uuid[7],
//                   g_beacon_cfg.uuid[8], g_beacon_cfg.uuid[9],
//                   g_beacon_cfg.uuid[10], g_beacon_cfg.uuid[11],
//                   g_beacon_cfg.uuid[12], g_beacon_cfg.uuid[13],
//                   g_beacon_cfg.uuid[14], g_beacon_cfg.uuid[15]);
//         debug_SPI("   major      = %u", g_beacon_cfg.major);
//         debug_SPI("   minor      = %u", g_beacon_cfg.minor);
//         debug_SPI("   txPower    = %d dBm", g_beacon_cfg.txPower);
//         debug_SPI("   interval   = %u ms", g_beacon_cfg.interval_ms);
//         // for (size_t i = 0; i < pkt_len; i++) {
//         //     debug_SPI("  [%02u] 0x%02X", (unsigned)i,
//         ((uint8_t*)&this->tx_frame)[i]);
//         // }
//         String hexLine;
//         for (size_t i = 0; i < pkt_len; i++)
//         {
//             char buf[5];
//             sprintf(buf, "%02X ", ((uint8_t *)&this->tx_frame)[i]);
//             hexLine += buf;
//         }
//         debug_SPI("FRAME HEX: %s", hexLine.c_str());

//         Fag_mask_regs.FAG_BEACON_RES = 1;
//     }

void handle_spi::setConfigBeacon()
{
  if (Fag_mask_regs.FAG_BEACON_RES == 2)
  {
    memset(&this->tx_frame, 0, sizeof(this->tx_frame));
    this->tx_frame.header.type = write_config;
    this->tx_frame.header.msk_regs = BEACON_CFG_RES;
    this->tx_frame.header.msk_regs.len = sizeof(beacon_cfg_t);
    memcpy(this->tx_frame.data, &g_beacon_cfg, sizeof(g_beacon_cfg));
    this->tx_frame.header.check_crc = SPI_master.calcCRC(
        this->tx_frame.data, this->tx_frame.header.msk_regs.len);
    const size_t pkt_len =
        sizeof(this->tx_frame.header) + this->tx_frame.header.msk_regs.len;
    Serial1.write((uint8_t *)&this->tx_frame, pkt_len);
    debug_SPI("Duc Thang >> Sent BEACON_CFG (len=%u)",
              (unsigned)this->tx_frame.header.msk_regs.len);
    debug_SPI("   SerialID      = %s", g_beacon_cfg.SerialID);
    debug_SPI("   Motion Timer  = %u ms", g_beacon_cfg.val_motion);
    debug_SPI("   Stand Timer   = %u ms", g_beacon_cfg.val_stand);
    debug_SPI("   UWB Channel   = %u", g_beacon_cfg.uwb_chan);
    debug_SPI("   UWB DataRate  = %u", g_beacon_cfg.uwb_datarate);
    // String hexLine;
    // for (size_t i = 0; i < pkt_len; i++) {
    //   char buf[4];
    //   sprintf(buf, "%02X ", ((uint8_t *)&this->tx_frame)[i]);
    //   hexLine += buf;
    //   if (i > 0 && i % 32 == 0)
    //     hexLine += "\n";
    // }
    // debug_SPI("FRAME HEX:\n%s", hexLine.c_str());
    Fag_mask_regs.FAG_BEACON_RES = 1;

    memset(&g_beacon_cfg, 0, sizeof(beacon_cfg_t));
  }
}

//   else if (Fag_mask_regs.FAG_BEACON_RES == 1)
//   {
//       memset((uint8_t *)&this->tx_frame, 0, sizeof(this->tx_frame));
//       this->tx_frame.header.type     = write_config;
//       this->tx_frame.header.msk_regs = BEACON_CFG_RES;
//       this->tx_frame.header.msk_regs.len = sizeof(beacon_cfg_t);
//       memcpy((uint8_t *)this->tx_frame.data, &g_beacon_cfg,
//       sizeof(g_beacon_cfg)); this->tx_frame.header.check_crc =
//       SPI_master.calcCRC(this->tx_frame.data,
//       this->tx_frame.header.msk_regs.len); const size_t pkt_len =
//       sizeof(this->tx_frame.header) + this->tx_frame.header.msk_regs.len;
//       Serial1.write((uint8_t *)&this->tx_frame, pkt_len);
//       debug_SPI("resync BEACON_CFG (len=%u)",
//       (unsigned)this->tx_frame.header.msk_regs.len);
//   }

handle_spi SPI_master;
