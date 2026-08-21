// NetBIOS.h  — NBNS responder for ESP32 (supports 0x20 & 0x21)
// Drop-in replacement (AsyncUDP-based)

#pragma once
#ifndef __ESPNBNS_h__
#define __ESPNBNS_h__

#include "Arduino.h"
#include "AsyncUDP.h"

class NetBIOS {
protected:
  AsyncUDP _udp;
  String   _name;     // Uppercase, truncated to ≤15 chars

  void _onPacket(AsyncUDPPacket &packet);

  // helpers
  static void _getnbname(const char *nbname, char *name, uint8_t maxlen); // decode first-level encoded NB name
  static inline void _wr16_be(uint8_t* d, uint16_t v) { d[0] = v >> 8; d[1] = v & 0xFF; }
  static inline void _wr32_be(uint8_t* d, uint32_t v) { d[0] = v >> 24; d[1] = v >> 16; d[2] = v >> 8; d[3] = v & 0xFF; }

  void _build_nb_name_answer(const uint8_t* qraw, size_t qlen, const IPAddress local_ip, uint8_t* out, size_t& outlen);
  void _build_nbstat_answer(const uint8_t* qraw, size_t qlen, uint8_t* out, size_t& outlen);

public:
  NetBIOS();
  ~NetBIOS();
  bool begin(const char *name);  // name ≤15 chars (will be uppercased)
  void end();
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_NETBIOS)
extern NetBIOS NBNS;
#endif

#endif // __ESPNBNS_h__
