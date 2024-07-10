/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/****************************************************************************
*
* This file is for gatt client. It can scan ble device, connect multiple devices,
* The gattc_multi_connect demo can connect three ble slaves at the same time.
* Modify the name of gatt_server demo named ESP_GATTS_DEMO_a,
* ESP_GATTS_DEMO_b and ESP_GATTS_DEMO_c,then run three demos,the gattc_multi_connect demo will connect
* the three gatt_server demos, and then exchange data.
* Of course you can also modify the code to connect more devices, we default to connect
* up to 4 devices, more than 4 you need to modify menuconfig.
*
****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gattc_api.h"
#include "esp_bt_main.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "types.h"
#include "constants.h"
#include "gap.h"
#include "gattc.h"

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    init_gap();
    init_gattc();

    start_scan();

    // ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    // if (ret){
    //     ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
    //     return;
    // }

    // ret = esp_ble_gattc_app_register(PROFILE_B_APP_ID);
    // if (ret){
    //     ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
    //     return;
    // }

    // ret = esp_ble_gattc_app_register(PROFILE_C_APP_ID);
    // if (ret){
    //     ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
    //     return;
    // }

    ret = esp_ble_gatt_set_local_mtu(200);
    if (ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", ret);
    }


}
