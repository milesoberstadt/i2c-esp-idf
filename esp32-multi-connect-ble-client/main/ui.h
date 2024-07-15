#if !defined(__UI_H__)
#define __UI_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"

#include "constants.h"
#include "led.h"
#include "button.h"
#include "gap.h"

#define UI_TAG "UI"

bool init_ui();

size_t get_selected_device();

void switch_selected_device();

#endif // __UI_H__
