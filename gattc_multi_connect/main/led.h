#if !defined(__LED_H__)
#define __LED_H__

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "constants.h"

void init_led();
void set_led(bool state);
bool get_led();

#endif // __LED_H__
