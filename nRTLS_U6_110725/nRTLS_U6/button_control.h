#include <Arduino.h>

#define switch_control_led 35

void sendUART(uint8_t *bufferSend, uint32_t len);
static inline void sendSwitchStateOverUart(bool pressed);
void read_button_state(void);
void setup_button_control();
void loop_button_control();