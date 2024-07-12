#if !defined(__BUTTON_H__)
#define __BUTTON_H__

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_timer.h"

#include "constants.h"
#include "gap.h"

#define BUTTON_TAG "BUTTON"

void init_button(gpio_isr_t isr_handler);

void IRAM_ATTR button_start_scan(void* arg);

#endif // __BUTTON_H__
