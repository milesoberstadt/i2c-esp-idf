#ifndef __TYPES_H__
#define __TYPES_H__

#include "esp_bt.h"
#include "esp_gattc_api.h"

#include <stdbool.h>

struct gattc_profile_inst {
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
    bool connected;
    bool discovered;
};

#endif