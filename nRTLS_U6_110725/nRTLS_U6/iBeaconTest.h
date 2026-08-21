#ifndef IBEACONTEST_H
#define IBEACONTEST_H

#include <Arduino.h>
#include "ArduinoJson.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEBeacon.h>

#define BEACON_UUID "27137124-3da6-4cce-ad72-f18e8ddbef86"
#define MAJOR (0)
#define MINOR (1)
/*
 *     BROADCAST_MODE          = 0B0000 (4 bit from bit 15 to 12 of highbyte major)
 *     DFU_MODE                = 0B0001 (4 bit from bit 15 to 12 of highbyte major)
 *     SETUPMOBILE_MODE        = 0B0010 (4 bit from bit 15 to 12 of highbyte major)
 *     SETUPUWB_MODE           = 0B0011 (4 bit from bit 15 to 12 of highbyte major)
 */
#define BROADCAST_MODE (0x0000)
#define DFU_MODE (0x1000)
#define SETUPMOBILE_MODE (0x2000)
#define SETUPUWB_MODE (0x3000)

/*
 *     MILIS_SETTAG            = 0B0100 (4 bit from 15 to 12 highbyte major of settag)
 *     SECONDS_SETTAG          = 0B0101 (4 bit from 15 to 12 highbyte major of settag)
 *     MINS_SETTAG             = 0B0110 (4 bit from 15 to 12 highbyte major of settag)
 *     HOURS_SETTAG            = 0B0111 (4 bit from 15 to 12 highbyte major of settag)
 */
#define MILIS_SETTAG (0x4000)
#define SECONDS_SETTAG (0x5000)
#define MINS_SETTAG (0x6000)
#define HOURS_SETTAG (0x7000)

/*
 *     MODE0                   = 0B00 (2 bit 15 and 14 of highbyte minor)
 *     MODE1                   = 0B01 (2 bit 15 and 14 of highbyte minor)
 *     MODE2                   = 0B10 (2 bit 15 and 14 of highbyte minor)
 *     MODE3                   = 0B11 (2 bit 15 and 14 of highbyte minor)
 */
#define MODE0 (0x0000)
#define MODE1 (0x4000)
#define MODE2 (0x8000)
#define MODE3 (0xC000)

class myIBeacon
{
public:
  myIBeacon();
  ~myIBeacon();
  struct
  {
    uint8_t active_Beacon;
    uint16_t hi_major;
    uint16_t lo_major;
    uint16_t hi_minor;
    uint16_t lo_minor;
    uint16_t time;
  } Beacon_config;

  
  void startIBeacon();
  void stopBeacon();
  void setIBeacon(uint16_t hi_major, uint16_t lo_major, uint16_t hi_minor, uint16_t lo_minor);
  void setTag(uint16_t hi_major, uint16_t lo_major, uint16_t hi_minor, uint16_t lo_minor);
  void checkBeaconConfig();
  void handle_beacon();
};

extern myIBeacon my_ibeacon;
#endif
