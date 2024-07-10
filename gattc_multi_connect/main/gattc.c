#include "gattc.h"

static struct gattc_profile_inst profiles[PROFILE_NUM];

void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    ESP_LOGI(GATTC_TAG, "EVT %d, gattc if %d, app_id %d", event, gattc_if, param->reg.app_id);

    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            profiles[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "Reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == profiles[idx].gattc_if) {
                if (profiles[idx].gattc_cb) {
                    profiles[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

void init_gattc() {
    //register the callback function to the gattc module
    esp_err_t ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "gattc register error, error code = %x", ret);
        return;
    }

    // register profiles 
    for (size_t i = 0; i < PROFILE_NUM; i++) {
        ret = esp_ble_gattc_app_register(i);
        if (ret){
            ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
            return;
        }
    }

    // init profiles array
    for (size_t i = 0; i < PROFILE_NUM; i++) {
        profiles[i].gattc_if = ESP_GATT_IF_NONE;
    }

}

size_t get_profile_count() {
    return sizeof(profiles) / sizeof(profiles[0]);
}

size_t next_available_profile_idx() {
    size_t count = get_profile_count();
    for (size_t i = 0; i < count; i++) {
        if (profiles[i].gattc_if == ESP_GATT_IF_NONE) {
            return i;
        }
    }
    return -1;
}

void open_profile(esp_bd_addr_t bda, esp_ble_addr_type_t ble_addr_type) {

    size_t idx = next_available_profile_idx();

    if (idx == -1) {
        ESP_LOGE(GATTC_TAG, "No available profile found");
        return;
    }

    ESP_LOGI(GATTC_TAG, "Opening profile at idx %d", idx);

    profiles[idx].status = CONNECTING;

    esp_err_t ret = esp_ble_gattc_open(
        profiles[idx].gattc_if, 
        bda, 
        ble_addr_type, 
        true);

        // todo : attach event handler

    if (ret){
        ESP_LOGE(GATTC_TAG, "gattc open error, error code = %x", ret);
    }

}