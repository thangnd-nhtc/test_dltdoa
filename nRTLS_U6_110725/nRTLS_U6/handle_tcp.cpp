#include "handle_tcp.h"
#include "DataBase.h"
#include "define.h"
#include "handle_config.h"
#include "handle_ethernet.h"
#include "handle_spi_master.h"
#include <WiFi.h>

typedef enum {
  Client_Wait_2s = 0,
  Client_Connecting,
  Client_Connected,
  Client_Disconnect,
} en_flag_tdoa_client;

uint8_t Tdoa_Client_Disconnect = 0;
en_flag_tdoa_client flag_tdoa_client_1 = Client_Connecting;
en_flag_tdoa_client flag_tdoa_client_2 = Client_Connecting;
WiFiClient Tdoa_client1;
WiFiClient Tdoa_client2;
WiFiClient Tdoa_client_1120;
en_flag_tdoa_client flag_tdoa_client_1120 = Client_Connecting;

TimeOutEvent KeepAliveTo;

handle_tcp::handle_tcp(/* args */) {}

handle_tcp::~handle_tcp() {}

bool handle_tcp::Tdoa_Client1_Status(void) {
  return (flag_tdoa_client_1 == Client_Connected) ? true : false;
}

bool handle_tcp::Tdoa_Client2_Status(void) {
  return (flag_tdoa_client_2 == Client_Connected) ? true : false;
}

bool handle_tcp::Tdoa_Client_1120_Status(void) {
  return (flag_tdoa_client_1120 == Client_Connected) ? true : false;
}

/* Hàm kiểm tra kết nối TCP STREAM
- Nếu rớt kết nối thì định thời 10s kết nối lại */
void handle_tcp::Tdoa_Client_Socket_Stream1_Handle(void) {
  static TimeOutEvent Tdoa_Client_To(2000);

  /*handle socket*/
  switch (flag_tdoa_client_1) {
  case Client_Wait_2s:
    if (Tdoa_Client_To.ToEExpired()) {
      flag_tdoa_client_1 = Client_Connecting;
    }
    break;
  case Client_Connecting:
    // if (!FileConfig.ConfigFile.STREAM.Enable)
    // 	break;
    if (handle_ethernet.lan_isconnect() != false ||
        handle_ethernet.wifi_isconnect() != false) {
    } else
      break;
    Tdoa_client1.setTimeout(0);
    Tdoa_client1.setNoDelay(true);
    {
      bool ok = false;
      for (int retry = 0; retry < 2 && !ok; retry++) {
        if (retry > 0) delay(100);
        ok = Tdoa_client1.connect(Config_Internet.TCP.Server,
                                  Config_Internet.TCP.Port, 1000);
      }
      if (!ok) {
        debug_TCP("Socket 1 connection failed\r\n");
        debug_TCP("Socket 1 connection failed %s, %d", Config_Internet.TCP.Server,
                  Config_Internet.TCP.Port);
        Tdoa_Client_To.ToEUpdate(2000);
        flag_tdoa_client_1 = Client_Wait_2s;
        break;
      }
    }
    debug_TCP("Socket 1 getNoDelay: %u\r\n", Tdoa_client1.getNoDelay());
    // Tdoa_client1.write("Stream1 Hello Server\r\n");
    debug_TCP("Socket 1 Connected\r\n");
    // DEBUG_TCP("Socket 1 Connected");
    /*Kết nối được thì vô hiệu hoa keep alive*/
    KeepAliveTo.ToEDisable();
    flag_tdoa_client_1 = Client_Connected;
    FlagcheckLed = true;
    break;
  case Client_Connected:
    if (!Tdoa_client1.connected() || Tdoa_Client_Disconnect == 1) {
      flag_tdoa_client_1 = Client_Disconnect;
    }
    break;
  case Client_Disconnect:
    Tdoa_Client_Disconnect = 0;
    debug_TCP("Socket 1 disconnected\r\n");
    debug_TCP("Socket 1 disconnected");
    Tdoa_client1.stop();
    Tdoa_Client_To.ToEUpdate(2000);
    flag_tdoa_client_1 = Client_Wait_2s;
    /*Nếu sau 15p không kết nối được TCP stream thì reset*/
    KeepAliveTo.ToEUpdate(15 * 60000);
    debug_TCP("Socket 1 reconnecting ... \r\n");
    FlagcheckLed = true;
    break;
  }
}

/* Hàm kiểm tra kết nối TCP STREAM
- Nếu rớt kết nối thì định thời 10s kết nối lại */
void handle_tcp::Tdoa_Client_Socket_Stream2_Handle(void) {
  static TimeOutEvent Tdoa_Client_To(2000);

  switch (flag_tdoa_client_2) {
  case Client_Wait_2s:
    if (Tdoa_Client_To.ToEExpired()) {
      flag_tdoa_client_2 = Client_Connecting;
    }
    break;
  case Client_Connecting:
    // if (!FileConfig.ConfigFile.STREAM.Enable)
    // 	break;
    // if (WiFi.status() != WL_CONNECTED)
    // 	break;

    if (handle_ethernet.lan_isconnect() != false ||
        handle_ethernet.wifi_isconnect() != false) {
    } else
      break;
    Tdoa_client2.setTimeout(0);
    Tdoa_client2.setNoDelay(true);
    {
      bool ok = false;
      for (int retry = 0; retry < 2 && !ok; retry++) {
        if (retry > 0) delay(100);
        ok = Tdoa_client2.connect(Config_Internet.TCP.Server,
                                  Config_Internet.TCP.Port, 1000);
      }
      if (!ok) {
        debug_TCP("Socket 2 connection failed\r\n");
        debug_TCP("Socket 2 connection failed %s, %d", Config_Internet.TCP.Server,
                  Config_Internet.TCP.Port);
        Tdoa_Client_To.ToEUpdate(2000);
        flag_tdoa_client_2 = Client_Wait_2s;
        break;
      }
    }
    debug_TCP("Socket 2 getNoDelay: %u\r\n", Tdoa_client2.getNoDelay());
    // Tdoa_client2.write("Stream2 Hello Server\r\n");
    debug_TCP("Socket 2 Connected\r\n");
    // debug_TCP("Socket 2 Connected");
    /*Kết nối được thì vô hiệu hoa keep alive*/
    KeepAliveTo.ToEDisable();
    flag_tdoa_client_2 = Client_Connected;
    FlagcheckLed = true;
    break;
  case Client_Connected:
    if (!Tdoa_client2.connected() || Tdoa_Client_Disconnect == 1) {
      flag_tdoa_client_2 = Client_Disconnect;
    }
    break;
  case Client_Disconnect:
    Tdoa_Client_Disconnect = 0;
    debug_TCP("Socket 2 disconnected\r\n");
    // debug_TCP("Socket 2 disconnected");
    Tdoa_client2.stop();
    Tdoa_Client_To.ToEUpdate(2000);
    flag_tdoa_client_2 = Client_Wait_2s;
    /*Nếu sau 15p không kết nối được TCP stream thì reset*/
    KeepAliveTo.ToEUpdate(15 * 60000);
    debug_TCP("Socket 2 reconnecting ... \r\n");
    FlagcheckLed = true;
    break;
  }
}

/* Hàm kiểm tra kết nối TCP STREAM 1120 */
void handle_tcp::Tdoa_Client_Socket_Stream_1120_Handle(void) {
  static TimeOutEvent Tdoa_Client_To(2000);

  switch (flag_tdoa_client_1120) {
  case Client_Wait_2s:
    if (Tdoa_Client_To.ToEExpired()) {
      flag_tdoa_client_1120 = Client_Connecting;
    }
    break;
  case Client_Connecting:
    if (handle_ethernet.lan_isconnect() != false ||
        handle_ethernet.wifi_isconnect() != false) {
    } else
      break;
    Tdoa_client_1120.setTimeout(0);
    Tdoa_client_1120.setNoDelay(true);
    // Port 1120 tĩnh theo yêu cầu
    {
      bool ok = false;
      for (int retry = 0; retry < 2 && !ok; retry++) {
        if (retry > 0) delay(100);
        ok = Tdoa_client_1120.connect(Config_Internet.TCP.Server, 1120, 1000);
      }
      if (!ok) {
        debug_TCP("Socket 1120 connection failed %s, 1120\r\n",
                  Config_Internet.TCP.Server);
        Tdoa_Client_To.ToEUpdate(2000);
        flag_tdoa_client_1120 = Client_Wait_2s;
        break;
      }
    }
    debug_TCP("Socket 1120 getNoDelay: %u\r\n", Tdoa_client_1120.getNoDelay());
    debug_TCP("Socket 1120 Connected\r\n");
    flag_tdoa_client_1120 = Client_Connected;
    break;
  case Client_Connected:
    if (!Tdoa_client_1120.connected() || Tdoa_Client_Disconnect == 1) {
      flag_tdoa_client_1120 = Client_Disconnect;
    }
    break;
  case Client_Disconnect:
    debug_TCP("Socket 1120 disconnected\r\n");
    Tdoa_client_1120.stop();
    Tdoa_Client_To.ToEUpdate(2000);
    flag_tdoa_client_1120 = Client_Wait_2s;
    debug_TCP("Socket 1120 reconnecting ... \r\n");
    break;
  }
}

void handle_tcp::Tdoa_Client_Socket_Stream_stop(void) {
  Tdoa_Client_Disconnect = 1;
}

// Hàm kiểm tra nếu có dữ liệu thì gửi TCP stream
/*
void handle_tcp::Tcp_Stream1_Core0(void)
{
        uint8_t retry = TCPStream_Retry;
        uint16_t Buff_num = 0;

        if ((Buff_num = Buff_is_available()) == 0)
                return;

        char Buff_send[Buff_num * BuffPacket_Size];
        int16_t Len_wait = 0, Len_send = 0;

        while (retry && this->Tdoa_Client1_Status())
        {
                //lấy dữ liệu lần đầu
                if (retry == TCPStream_Retry)
                        Len_wait = TakeBuff(Buff_send, Buff_num);

                Len_send = Tdoa_client1.write(Buff_send, Len_wait);

                if (Len_send == Len_wait)
                {
                        // TDOA_Stream_Dbg("Tag send OK");
                        return;
                }
                if (--retry)
                        delay(10);
        }
        if (Len_send != Len_wait && flag_tdoa_client_2 == Client_Connected)
        {
                flag_tdoa_client_1 = Client_Disconnect;
                // TDOA_Stream_Dbg("Stream 1 Send error %d packet.. %llu/%llu",
Len_wait / BuffPacket_Size, Tdoa_lost_packet, Tdoa_total_packet);
                // log_file_printf("Stream 1 Send error %d packet", Len_wait /
BuffPacket_Size);
        }
}

void handle_tcp::Tcp_Stream2_Core0(void)
{
        uint8_t retry = TCPStream_Retry;
        uint16_t Buff_num = 0;
        if (BuffSize_protect)
                Buff_num = 1;
        else
                return;

        char Buff_send[Buff_num * BuffPacket_Size];
        int16_t Len_wait = 0, Len_send = 0;

        while (retry && Tdoa_Client2_Status())
        {
                //lấy dữ liệu lần đầu
                if (retry == TCPStream_Retry)
                {
                        Len_wait = BuffSize_protect;
                        BuffSize_protect = 0;
                        memcpy(Buff_send, BuffShare_protect, Len_wait);
                }

                Len_send = Tdoa_client2.write(Buff_send, Len_wait);

                if (Len_send == Len_wait)
                {
                        TDOA_Stream_Dbg("Sync send OK");
                        return;
                }
                if (--retry)
                        delay(10);
        }
        if (Len_send != Len_wait && flag_tdoa_client_1 == Client_Connected)
        {
                flag_tdoa_client_2 = Client_Disconnect;
                TDOA_Stream_Dbg("Stream 2 Send error %d packet.. %llu/%llu",
Len_wait / BuffPacket_Size, Tdoa_lost_packet, Tdoa_total_packet);
                // log_file_printf("Stream 2 Send error %d packet", Len_wait /
BuffPacket_Size);
        }
}
// void handle_tcp::sendStream2(uint8_t* dataSend,size_t len)
// {
// 	size_t Len_send = Tdoa_client2.write((uint8_t*)dataSend,len);
// }
*/
void handle_tcp::sendTCP(uint8_t *dataSend, size_t len) {
  // 1. Filter trước — tạo bản copy tương thích định dạng cũ (chỉ copy RAM, cực
  // nhanh)
  uint8_t filteredData[300];
  size_t filteredLen = 0;
  int commaCount = 0;
  bool isType0or5 = false;

  if (len > 0) {
    isType0or5 = (dataSend[0] == '0' || dataSend[0] == '5');
  }

  for (size_t i = 0; i < len; i++) {
    if (dataSend[i] == ':') {
      filteredData[filteredLen++] = ':';
      if (i + 1 < len && dataSend[i + 1] == '\r') {
        filteredData[filteredLen++] = '\r';
        if (i + 2 < len && dataSend[i + 2] == '\n') {
          filteredData[filteredLen++] = '\n';
        }
      }
      break;
    }
    if (dataSend[i] == ',') {
      commaCount++;
      if (isType0or5 && commaCount == 17) {
        filteredData[filteredLen++] = ':';
        filteredData[filteredLen++] = '\r';
        filteredData[filteredLen++] = '\n';
        break;
      }
    }
    filteredData[filteredLen++] = dataSend[i];
  }

  // 2. Gửi Stream1/Stream2 (7777) TRƯỚC
  if (flag_tdoa_client_1 == Client_Connected)
    Tdoa_client1.write((uint8_t *)filteredData, filteredLen);
  else if (flag_tdoa_client_2 == Client_Connected)
    Tdoa_client2.write((uint8_t *)filteredData, filteredLen);

  // 3. Gửi Full Data qua Socket 1120 SAU
  if (flag_tdoa_client_1120 == Client_Connected)
    Tdoa_client_1120.write((uint8_t *)dataSend, len);
}

// void handle_tcp::sendStream2(uint8_t* dataSend,size_t len)
// {
// 	size_t Len_send = Tdoa_client2.write((uint8_t*)dataSend,len);
// }

WiFiClient Twr_client_2011;
en_flag_tdoa_client flag_twr_client_2011 = Client_Connecting;

bool handle_tcp::Twr_Client_2011_Status(void) {
  return (flag_twr_client_2011 == Client_Connected) ? true : false;
}

/* TCP Stream port 2011 cho BSS-TWR Result */
void handle_tcp::Twr_Client_Socket_Stream_2011_Handle(void) {
  static TimeOutEvent Twr_Client_To(2000);

  switch (flag_twr_client_2011) {
  case Client_Wait_2s:
    if (Twr_Client_To.ToEExpired()) {
      flag_twr_client_2011 = Client_Connecting;
    }
    break;
  case Client_Connecting:
    if (handle_ethernet.lan_isconnect() != false ||
        handle_ethernet.wifi_isconnect() != false) {
    } else
      break;
    Twr_client_2011.setTimeout(0);
    Twr_client_2011.setNoDelay(true);
    {
      bool ok = false;
      for (int retry = 0; retry < 2 && !ok; retry++) {
        if (retry > 0) delay(100);
        ok = Twr_client_2011.connect(Config_Internet.TCP.Server, 2011, 1000);
      }
      if (!ok) {
        debug_TCP("Socket 2011 (TWR) connection failed %s, 2011\r\n",
                  Config_Internet.TCP.Server);
        Twr_Client_To.ToEUpdate(2000);
        flag_twr_client_2011 = Client_Wait_2s;
        break;
      }
    }
    debug_TCP("Socket 2011 (TWR) Connected\r\n");
    flag_twr_client_2011 = Client_Connected;
    break;
  case Client_Connected:
    if (!Twr_client_2011.connected() || Tdoa_Client_Disconnect == 1) {
      flag_twr_client_2011 = Client_Disconnect;
    }
    break;
  case Client_Disconnect:
    debug_TCP("Socket 2011 (TWR) disconnected\r\n");
    Twr_client_2011.stop();
    Twr_Client_To.ToEUpdate(2000);
    flag_twr_client_2011 = Client_Wait_2s;
    debug_TCP("Socket 2011 (TWR) reconnecting ...\r\n");
    break;
  }
}

void handle_tcp::sendTWR(uint8_t *dataSend, size_t len) {
  if (flag_twr_client_2011 != Client_Connected ||
      !Twr_client_2011.connected()) {
    return;
  }

  size_t written = Twr_client_2011.write(dataSend, len);
  if (written != len) {
    debug_TCP("Socket 2011 (TWR) short write: %u/%u\r\n",
              (unsigned int)written, (unsigned int)len);
    flag_twr_client_2011 = Client_Disconnect;
  }
}

handle_tcp _handle_tcp;
