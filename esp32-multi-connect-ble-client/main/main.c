#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"

#include "constants.h"
#include "gap.h"
#include "gattc.h"
#include "preferences.h"
#include "ui.h"
#include "ble.h"

#define MAIN_TAG "ESP32_MULTI_CONNECT_BLE_CLIENT"

void app_main(void)
{
    bool ret = init_preferences();
    if (!ret) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize preferences");
        return;
    }

    ret = init_ble();
    if (!ret) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize BLE");
        return;
    }

    ret = init_gap();
    if (!ret) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize GAP");
        return;
    }

    ret = init_gattc();
    if (!ret) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize GATTC");
        return;
    }

    ret = init_ui();
    if (!ret) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize UI");
        return;
    }
}
