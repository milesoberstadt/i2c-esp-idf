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
#include "types.h"
#include "gap.h"

#define BUTTON_TAG "BUTTON"

bool init_button(button_config_t *config);

#endif // __BUTTON_H__
