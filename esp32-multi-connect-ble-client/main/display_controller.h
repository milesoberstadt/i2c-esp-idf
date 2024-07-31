#ifndef __DISPLAY_CONTROLLER_H__
#define __DISPLAY_CONTROLLER_H__

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "constants.h"
#include "types.h"
#include "i2c_messages.h"

#define DISPLAY_TAG "DISPLAY"

bool init_display();

void display_sleep();

void display_wake();

#endif // __DISPLAY_CONTROLLER_H__
