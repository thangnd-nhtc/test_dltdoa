#ifndef __TCP_HANDLE_H
#define __TCP_HANDLE_H

#include "Arduino.h"

class handle_tcp {
private:
  /* data */
public:
  handle_tcp(/* args */);
  ~handle_tcp();

  bool Tdoa_Client1_Status(void);
  bool Tdoa_Client2_Status(void);
  bool Tdoa_Client_1120_Status(void);
  bool Twr_Client_2011_Status(void);
  void Tdoa_Client_Socket_Stream1_Handle(void);
  void Tdoa_Client_Socket_Stream2_Handle(void);
  void Tdoa_Client_Socket_Stream_1120_Handle(void);
  void Twr_Client_Socket_Stream_2011_Handle(void);
  void Tdoa_Client_Socket_Stream_stop(void);

  void Tcp_Stream1_Core0(void);
  void Tcp_Stream2_Core0(void);

  void sendTCP(uint8_t *dataSend, size_t len);
  void sendTWR(uint8_t *dataSend, size_t len);
  // void sendStream2(uint8_t* dataSend, size_t len);
};
extern handle_tcp _handle_tcp;

#endif
