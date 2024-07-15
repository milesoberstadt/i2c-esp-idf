#include <stdio.h>
#include <stdbool.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_log.h"

#include "constants.h"
#include "gap.h"
#include "gattc.h"
#include "led.h"
#include "button.h"
#include "preferences.h"

#define MAIN_TAG "ESP32_MULTI_CONNECT_BLE_CLIENT"

void init_bluetooth() {
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(MAIN_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(MAIN_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
}

void app_main(void)
{
    bool ret = init_preferences();
    if (!ret) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize preferences");
        return;
    }

    init_bluetooth();
    init_gap();
    init_gattc();

    init_led();

    init_button(button_start_scan);

}
