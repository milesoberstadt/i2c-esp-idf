#if !defined(__LED_H__)
#define __LED_H__

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_check.h"

#include "constants.h"

#define LED_TAG "LED"

void init_led();
void set_led(bool state);
void start_led_blink(int delay_ms);
void stop_led_blink();
bool get_led();

#endif // __LED_H__
