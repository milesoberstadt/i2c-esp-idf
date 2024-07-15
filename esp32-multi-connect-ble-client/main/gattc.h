#if !defined(__GATTC_H__)
#define __GATTC_H__

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "esp_bt.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "esp_check.h"

#include "types.h"
#include "constants.h"
#include "uuid128.h"
#include "led.h"

#define GATTC_TAG "ESP32_MCBC_GATTC"

void init_gattc();

void open_profile(esp_bd_addr_t bda, esp_ble_addr_type_t ble_addr_type, size_t idx);

#endif // __GATTC_H__
