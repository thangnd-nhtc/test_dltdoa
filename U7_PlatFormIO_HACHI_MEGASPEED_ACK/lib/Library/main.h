// include/main.h
#pragma once
#include <Arduino.h>

// Nếu biến có thể đổi trong ISR / Task khác core → dùng volatile
extern volatile bool stat_btn;
extern unsigned long  millis_on_led;
extern unsigned long  timeout_on_led;
extern volatile bool flag_timeout_led;
