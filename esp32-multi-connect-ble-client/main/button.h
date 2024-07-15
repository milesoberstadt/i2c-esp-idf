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
#define DEBOUNCE_TIME_MS 50

typedef void (*button_callback)(void);

typedef struct button_config_t {
    gpio_num_t gpio_num;
    button_callback press_callback;
    button_callback long_press_callback;
} button_config_t;

typedef struct button_state_t {
    gpio_num_t gpio_num;
    button_callback press_callback;
    button_callback long_press_callback;
    TickType_t press_start_time;
    bool is_pressed;
} button_state_t;

bool init_button(button_config_t *config);

#endif // __BUTTON_H__
