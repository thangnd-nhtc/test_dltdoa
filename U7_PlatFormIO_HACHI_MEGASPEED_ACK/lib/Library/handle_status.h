#ifndef _HANDLE_STATUS_H__
#define _HANDLE_STATUS_H__

#include "TimedBlink.h"
#include "define.h"

extern status_ledINTERNET internet_ready_f;
extern status_ledDW decawave_ready_f;
extern status_ledPower power_ready_f;

class handle_status
{
private:
public:
    handle_status(/* args */);
    ~handle_status();

    void begin(void);
    void loop(void);

    void off_all(void); // tắt tất cả led

    void internet(status_ledINTERNET status);
    void decawace(status_ledDW status);
    void power(status_ledPower status);
};

extern handle_status led_status;

#endif
