
#ifndef __TimedBlink__H__
#define __TimedBlink__H__

#include <Arduino.h>

enum blink_t {BLINK_ON, BLINK_OFF};
typedef enum 
{
  led_error_DW = 0,
  led_ok_DW,
  led_sync_ERROR_DW,
  led_clock_error_DW
} status_ledDW;

typedef enum 
{
  ledP_off = 0,
  ledP_on,
  ledP_blink
} status_ledPower;

typedef enum 
{
  led_internet_fail = 0,
  led_internet_ok,
  led_fail_server,
  led_fail_TCP,
  led_fail_MQTT
} status_ledINTERNET;

class TimedBlink {
  private:
    unsigned long m_blinkTime;
    int m_onForTime;
    int m_offForTime;
    blink_t m_blinkState;
    short m_pin;
    int m_resolution;

    void reset();

  public:

    TimedBlink(int pin);
    void blink(int on_for, int off_for);
    void blink();
    void setOnTime(int ms);
    void setOffTime(int ms);
    void setBlinkState(blink_t state);
    void blinkDelay(int d);
    void blinkOff();
};

#endif // __TimedBlink__H__
