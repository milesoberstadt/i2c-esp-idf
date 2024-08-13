#if !defined(__DISPLAY_H__)
#define __DISPLAY_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ssd1306.h"
#include "font8x8_basic.h"

#define TAG "SSD1306"

void init_display();

void display_text(char *text, size_t len);

#endif // __DISPLAY_H__
