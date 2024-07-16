#if !defined(__GATTC_H__)
#define __GATTC_H__

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "esp_check.h"

#include "constants.h"
#include "uuid128.h"
#include "led.h"
#include "devices.h"

#define GATTC_TAG "ESP32_MCBC_GATTC"
#define DEVICE_TAG_SIZE 16

struct gattc_profile_inst {
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
    esp_ble_addr_type_t ble_addr_type;
    bool connected;
    bool discovered;
};

bool init_gattc();

void open_profile(esp_bd_addr_t bda, esp_ble_addr_type_t ble_addr_type, size_t idx);

bool is_profile_active(size_t idx);

#endif // __GATTC_H__
