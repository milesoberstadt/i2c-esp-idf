#include <stdbool.h>

#include "esp_log.h"

#include "constants.h"
#include "gap.h"
#include "gattc.h"
#include "devices.h"
#include "ui.h"
#include "ble.h"
#include "i2c_master.h"

#define MAIN_TAG "ESP32_MULTI_CONNECT_BLE_CLIENT"

void app_main(void)
{

    ESP_LOGI(MAIN_TAG, "Start program initialization");

    bool ret = init_devices();
    if (!ret) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize devices (storage)");
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

    ret = i2c_init();
    if (!ret) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize I2C Master");
        return;
    }

    ESP_LOGI(MAIN_TAG, "Initialization complete");

    ESP_LOGI(MAIN_TAG, "Trying to connecting to devices ...");
    connect_all_devices();

}
