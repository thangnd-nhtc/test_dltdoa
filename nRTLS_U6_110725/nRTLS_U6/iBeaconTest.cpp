#include "iBeaconTest.h"
#include "define.h"
#include "handle_config.h"
#include "handle_SPIFFS.h"
#include "Ticker.h"

Ticker time_beacon;
BLEAdvertising *pAdvertising;

uint16_t arrUUID[2];

extern "C"
{
  void startIBeacon();
}

myIBeacon ::myIBeacon()
{
}

myIBeacon ::~myIBeacon()
{
}

void stopbeacon()
{
  debug_beacon("stop beacon");
  my_ibeacon.Beacon_config.active_Beacon = 0;
  my_ibeacon.stopBeacon();
  time_beacon.detach();
}

void myIBeacon::startIBeacon()
{
  debug_beacon("iBeacon start");
  BLEDevice::init("myEsp32");

  BLEServer *pServer = BLEDevice::createServer(); //

  BLEBeacon myBeacon;
  myBeacon.setManufacturerId(0x4C00);
  myBeacon.setProximityUUID(BLEUUID("27137124-3da6-4cce-ad72-f18e8ddbef86"));
  myBeacon.setMajor(arrUUID[MAJOR]);
  myBeacon.setMinor(arrUUID[MINOR]);
  myBeacon.setSignalPower(0xC8);

  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x04);
  advertisementData.setManufacturerData(myBeacon.getData());

  pAdvertising = pServer->getAdvertising();
  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->setScanResponseData(advertisementData);
  pAdvertising->start();
}

void myIBeacon::stopBeacon()
{
  //  BLEAdvertising *pAdvertising;
  pAdvertising->stop();
}

void myIBeacon::setIBeacon(uint16_t hi_major, uint16_t lo_major, uint16_t hi_minor, uint16_t lo_minor)
{
  switch (hi_major)
  {
  case BROADCAST_MODE:
    arrUUID[MAJOR] = BROADCAST_MODE | lo_major;
    switch (hi_minor)
    {
    case MODE0:
      arrUUID[MINOR] = MODE0 | lo_minor;
      debug_beacon(" Set UWB");
      break;
    case MODE1:
      arrUUID[MINOR] = MODE1 | lo_minor;
      debug_beacon(" Set UWB and LED");
      break;
    case MODE2:
      arrUUID[MINOR] = MODE2 | lo_minor;
      debug_beacon(" Set UWB and Speaker");
      break;
    case MODE3:
      arrUUID[MINOR] = MODE3 | lo_minor;
      debug_beacon(" Set UWB,LED and Speaker");
      break;
    default:
      arrUUID[MAJOR] = 0x00;
      arrUUID[MINOR] = 0x00;
      debug_beacon(" Not mode of Broadcast");
      break;
    }
    break;
  case DFU_MODE:
    arrUUID[MAJOR] = DFU_MODE | lo_major;
    arrUUID[MINOR] = MODE0 | lo_minor;
    debug_beacon(" Set DFU");
    break;
  case SETUPMOBILE_MODE:
    arrUUID[MAJOR] = SETUPMOBILE_MODE | lo_major;
    arrUUID[MINOR] = MODE0 | lo_minor;
    debug_beacon(" Setup Mobile");
    break;
  case SETUPUWB_MODE:
    arrUUID[MAJOR] = SETUPUWB_MODE | lo_major;
    switch (hi_minor)
    {
    case MODE0:
      arrUUID[MINOR] = MODE0 | lo_minor;
      debug_beacon(" Set channel");
      break;
    case MODE1:
      arrUUID[MINOR] = MODE1 | lo_minor;
      debug_beacon(" Set Preamble_Code");
      break;
    case MODE2:
      arrUUID[MINOR] = MODE2 | lo_minor;
      debug_beacon(" Set PG_Delay");
      break;
    case MODE3:
      arrUUID[MINOR] = MODE3 | lo_minor;
      debug_beacon(" Set TX_power");
      break;
    default:
      arrUUID[MAJOR] = 0x00;
      arrUUID[MINOR] = 0x00;
      debug_beacon(" Not mode of SetUWB");
      break;
    }
    break;
  default:
    arrUUID[MAJOR] = 0x00;
    arrUUID[MINOR] = 0x00;
    debug_beacon(" Not Major");
    break;
  }
}

// void myIBeacon::setTag(uint16_t hi_major, uint16_t lo_major, uint16_t hi_minor, uint16_t lo_minor, uint16_t *arrUUID)
// {
//   switch (hi_minor)
//   {
//   case MODE1:
//     arrUUID[MINOR] = MODE1 | lo_minor;
//     switch (hi_major)
//     {
//     case MILIS_SETTAG:
//       arrUUID[MAJOR] = MILIS_SETTAG | lo_major;
//       debug_beacon("Set motion interval(miliseconds)");
//       break;
//     case SECONDS_SETTAG:
//       arrUUID[MAJOR] = SECONDS_SETTAG | lo_major;
//       debug_beacon(" Set motion interval(seconds)");
//       break;
//     case MINS_SETTAG:
//       arrUUID[MAJOR] = MINS_SETTAG | lo_major;
//       debug_beacon(" Set motion interval(minutes)");
//       break;
//     case HOURS_SETTAG:
//       arrUUID[MAJOR] = HOURS_SETTAG | lo_major;
//       debug_beacon(" Set motion interval(hours)");
//       break;
//     default:
//       arrUUID[MAJOR] = 0x00;
//       arrUUID[MINOR] = 0x00;
//       debug_beacon(" Not Motion");
//       break;
//     }
//     break;

//   case MODE2:
//     arrUUID[MINOR] = MODE2 | lo_minor;
//     switch (hi_major)
//     {
//     case MILIS_SETTAG:
//       arrUUID[MAJOR] = MILIS_SETTAG | lo_major;
//       debug_beacon(" Set standby interval(miliseconds)");
//       break;
//     case SECONDS_SETTAG:
//       arrUUID[MAJOR] = SECONDS_SETTAG | lo_major;
//       debug_beacon(" Set standby interval(seconds)");
//       break;
//     case MINS_SETTAG:
//       arrUUID[MAJOR] = MINS_SETTAG | lo_major;
//       debug_beacon(" Set standby interval(minutes)");
//       break;
//     case HOURS_SETTAG:
//       arrUUID[MAJOR] = HOURS_SETTAG | lo_major;
//       debug_beacon(" Set standby interval(hours)");
//       break;
//     default:
//       arrUUID[MAJOR] = 0x00;
//       arrUUID[MINOR] = 0x00;
//       debug_beacon(" Not Standby");
//       break;
//     }
//     break;
//   case MODE3:
//     arrUUID[MINOR] = MODE3 | lo_minor;
//     switch (hi_major)
//     {
//     case MILIS_SETTAG:
//       arrUUID[MAJOR] = MILIS_SETTAG | lo_major;
//       debug_beacon(" Set sensor interval(miliseconds)");
//       break;
//     case SECONDS_SETTAG:
//       arrUUID[MAJOR] = SECONDS_SETTAG | lo_major;
//       debug_beacon(" Set sensor interval(seconds)");
//       break;
//     case MINS_SETTAG:
//       arrUUID[MAJOR] = MINS_SETTAG | lo_major;
//       debug_beacon(" Set sensor interval(minutes)");
//       break;
//     case HOURS_SETTAG:
//       arrUUID[MAJOR] = HOURS_SETTAG | lo_major;
//       debug_beacon(" Set sensor interval(hours)");
//       break;
//     default:
//       arrUUID[MAJOR] = 0x00;
//       arrUUID[MINOR] = 0x00;
//       debug_beacon(" Not Sensor");
//       break;
//     }
//     break;
//   default:
//     arrUUID[MAJOR] = 0x00;
//     arrUUID[MINOR] = 0x00;
//     debug_beacon(" Not SetTag");
//     break;
//   }
// }

void myIBeacon::checkBeaconConfig()
{
  String dataInfor = _handle_SPIFFS.readInformations(BEACONCONFIG);
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, dataInfor);

  if (error)
  {
    debug_beacon("deserializeJson() beacon failed: %s", error.c_str());
    my_ibeacon.Beacon_config.active_Beacon = 0;
    return;
  }

  // my_ibeacon.Beacon_config.arrUUID = doc["arrUUID"];
  // debug_beacon("arrUUID:%lu", my_ibeacon.Beacon_config.arrUUID);
  my_ibeacon.Beacon_config.active_Beacon = doc["ActiveBeacon"];
  debug_beacon("ActiveBeacon:%d", my_ibeacon.Beacon_config.active_Beacon);
  my_ibeacon.Beacon_config.hi_major = doc["HighMajor"];
  debug_beacon("HighMajor:%lu", my_ibeacon.Beacon_config.hi_major);
  my_ibeacon.Beacon_config.lo_major = doc["LowMajor"];
  debug_beacon("LowMajor:%lu", my_ibeacon.Beacon_config.lo_major);
  my_ibeacon.Beacon_config.hi_minor = doc["HighMinor"];
  debug_beacon("HighMinor:%lu", my_ibeacon.Beacon_config.hi_minor);
  my_ibeacon.Beacon_config.lo_minor = doc["LowMinor"];
  debug_beacon("LowMinor:%lu", my_ibeacon.Beacon_config.lo_minor);
}

void myIBeacon::handle_beacon()
{
  my_ibeacon.setIBeacon(my_ibeacon.Beacon_config.hi_major, my_ibeacon.Beacon_config.lo_major,
                        my_ibeacon.Beacon_config.hi_minor, my_ibeacon.Beacon_config.lo_minor);
  // if (my_ibeacon.Beacon_config.active_Beacon)
  if (my_ibeacon.Beacon_config.active_Beacon == 1)
  {
    debug_beacon("start beacon");
    my_ibeacon.startIBeacon();
    if (my_ibeacon.Beacon_config.time > 0)
      time_beacon.attach(my_ibeacon.Beacon_config.time, stopbeacon);
  }
  else
    my_ibeacon.stopBeacon();
}
myIBeacon my_ibeacon;
