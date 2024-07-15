#if !defined(__GAP_H__)
#define __GAP_H__

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "esp_check.h"

#include "constants.h"
#include "uuid128.h"
#include "gattc.h"
#include "led.h"
#include "ui.h"

#define GAP_TAG "ESP32_MCBC_GAP"

void init_gap();
void start_scan();

bool get_is_scanning();

#endif // __GAP_H__
