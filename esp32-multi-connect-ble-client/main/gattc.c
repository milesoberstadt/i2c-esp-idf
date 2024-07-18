#include "gattc.h"

static gattc_profile_inst profiles[MAX_DEVICES];

size_t get_profile_count() {
    return sizeof(profiles) / sizeof(profiles[0]);
}

bool is_profile_available(size_t idx) {
    return !profiles[idx].connected;
}

size_t next_available_profile_idx() {
    size_t count = get_profile_count();
    for (size_t i = 0; i < count; i++) {
        if (is_profile_available(i)) {
            return i;
        }
    }
    return -1;
}

void generate_device_tag(size_t idx, char *tag) {
    snprintf(tag, DEVICE_TAG_SIZE, "DEVICE_%d", idx);
}

size_t get_idx_by_gattc_if(esp_gatt_if_t gattc_if) {
    size_t idx;
    for (idx = 0; idx < MAX_DEVICES; idx++) {
        if (profiles[idx].gattc_if == gattc_if) {
            return idx;
        }
    }
    return -1;
}


void connection_start_handler(size_t idx) {
    start_led_blink(idx, -1, 300);
}

void connection_end_handler(size_t idx) {
    stop_led_blink(idx);
    set_led(idx, false);
    profiles[idx].connected = false;
    profiles[idx].discovered = false;
    profiles[idx].gattc_if = ESP_GATT_IF_NONE;
    profiles[idx].device_type = UNKNOWN_DEVICE;
}

void connection_success_handler(size_t idx) {
    stop_led_blink(idx);
    set_led(idx, true);

    bool ret = add_device(profiles[idx].remote_bda, profiles[idx].ble_addr_type, profiles[idx].device_type, idx);
    if (!ret) {
        ESP_LOGE(GATTC_TAG, "Failed to save device");
    }

}

void disconnected_handler(size_t idx) {

    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    esp_err_t ret = esp_ble_gattc_app_unregister(profiles[idx].gattc_if);
    if (ret){
        ESP_LOGE(DEVICE_TAG, "gattc app unregister error, error code = %x", ret);
    }

    connection_end_handler(idx);
    
    ESP_LOGI(DEVICE_TAG, "DEVICE DISCONNECTED");
}

void connection_oppened_handler(size_t idx, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *p_data) {
    profiles[idx].conn_id = p_data->open.conn_id;
    profiles[idx].connected = true;

    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    ESP_LOGI(DEVICE_TAG, "DEVICE CONNECTED SUCCESSFULLY");
    ESP_LOGI(DEVICE_TAG, "conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
    
    esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
    if (mtu_ret){
        ESP_LOGE(DEVICE_TAG, "config MTU error, error code = %x", mtu_ret);
    }

}

void gattc_profile_callback(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{

    size_t idx = get_idx_by_gattc_if(gattc_if);

    if (idx == -1) {
        ESP_LOGE(GATTC_TAG, "Profile not found for gattc_if %d", gattc_if);
        return;
    }

    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    esp_err_t ret;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(DEVICE_TAG, "Device registered");
        ESP_LOGI(DEVICE_TAG, "Opening profile ...");

        ret = esp_ble_gattc_open(
                profiles[idx].gattc_if, 
                profiles[idx].remote_bda,
                profiles[idx].ble_addr_type, 
                true);

        if (ret){
            ESP_LOGE(GATTC_TAG, "gattc open error, error code = %x", ret);
        }

        break;
    case ESP_GATTC_CONNECT_EVT:
        break;
    case ESP_GATTC_OPEN_EVT:

        if (p_data->open.status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG, "connection failed, status %d", p_data->open.status);
            disconnected_handler(idx);
            break;
        } 

        connection_oppened_handler(idx, gattc_if, p_data);

        break;
    case ESP_GATTC_CFG_MTU_EVT:

        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG,"Config mtu failed");
            disconnected_handler(idx);
        }

        ESP_LOGI(DEVICE_TAG, "mut set : Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        ESP_LOGI(DEVICE_TAG, "Looking for services...");

        ret = esp_ble_gattc_search_service(
            gattc_if, 
            param->cfg_mtu.conn_id, 
            &UUID_M_NODE
        );
        
        if (ret){
            ESP_LOGE(DEVICE_TAG, "search service failed, error status = %x", ret);
        }

        break;
    case ESP_GATTC_SEARCH_RES_EVT: {

        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_128 && compare_uuid(UUID_M_NODE.uuid.uuid128, p_data->search_res.srvc_id.uuid.uuid.uuid128)) {
            ESP_LOGI(DEVICE_TAG, "MATCHING SERVICE FOUND");
            profiles[idx].service_start_handle = p_data->search_res.start_handle;
            profiles[idx].service_end_handle = p_data->search_res.end_handle;
            profiles[idx].discovered = true;
        } else {
            ESP_LOGI(DEVICE_TAG, "Service found, but UUID don't match");
        }

        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        ESP_LOGI(DEVICE_TAG, "Service search completed: conn_id = %x, status %d", p_data->search_cmpl.conn_id, p_data->search_cmpl.status);

        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG, "search service failed, error status = %d", p_data->search_cmpl.status);
            disconnected_handler(idx);
            break;
        }
        if (profiles[idx].discovered){

            ESP_LOGI(DEVICE_TAG, "Service found, looking for characteristics");

            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     profiles[idx].service_start_handle,
                                                                     profiles[idx].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(DEVICE_TAG, "esp_ble_gattc_get_attr_count error");
                disconnected_handler(idx);
                break;
            }

            ESP_LOGI(DEVICE_TAG, "Characteritics found : %d", count);

            if (count > 0) {

                esp_gattc_char_elem_t *char_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);

                if (!char_result){
                    ESP_LOGE(DEVICE_TAG, "gattc no mem");
                    disconnected_handler(idx);
                    break;
                }else {

                    status = esp_ble_gattc_get_char_by_uuid(gattc_if,
                                                            p_data->search_cmpl.conn_id,
                                                            profiles[idx].service_start_handle,
                                                            profiles[idx].service_end_handle,
                                                            UUID_GYRO_CHAR,
                                                            char_result,
                                                            &count);

                    if (status != ESP_GATT_OK){
                        ESP_LOGE(DEVICE_TAG, "esp_ble_gattc_get_char_by_uuid error, error status = %x", status);
                        free(char_result);
                        char_result = NULL;
                        disconnected_handler(idx);
                        break;
                    }

                    ESP_LOGI(DEVICE_TAG, "char_handle %d char property %x", char_result[0].char_handle, char_result[0].properties);

                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        ESP_LOGI(DEVICE_TAG, "char property has notify");
                        profiles[idx].char_handle = char_result[0].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, profiles[idx].remote_bda, char_result[0].char_handle);
                    } else {
                        ESP_LOGE(DEVICE_TAG, "no char property has notify");
                        disconnected_handler(idx);
                    }
                }
                free(char_result);
            } else {
                ESP_LOGE(DEVICE_TAG, "no char found");
                disconnected_handler(idx);
            }
        }
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
            disconnected_handler(idx);
            break;
        }
        uint16_t count = 0;
        uint16_t notify_en = 1;
        ESP_LOGI(DEVICE_TAG, "Registering notify ...");
        esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     profiles[idx].conn_id,
                                                                     ESP_GATT_DB_DESCRIPTOR,
                                                                     profiles[idx].service_start_handle,
                                                                     profiles[idx].service_end_handle,
                                                                     profiles[idx].char_handle,
                                                                     &count);
        if (ret_status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG, "esp_ble_gattc_get_attr_count error");
            disconnected_handler(idx);
        }
        if (count > 0){
            esp_gattc_descr_elem_t* descr_result = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t) * count);
            if (!descr_result){
                ESP_LOGE(DEVICE_TAG, "malloc error, gattc no mem");
                disconnected_handler(idx);
            }else{
                ESP_LOGI(DEVICE_TAG, "Getting attribute descriptor %d", count);
                ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                     profiles[idx].conn_id,
                                                                     p_data->reg_for_notify.handle,
                                                                     notify_descr_uuid,
                                                                     descr_result,
                                                                     &count);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(DEVICE_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                    disconnected_handler(idx);
                }

                ESP_LOGI(DEVICE_TAG, "descr_handle %d descr uuid %x", descr_result[0].handle, descr_result[0].uuid.len);
                if (count > 0 && descr_result[0].uuid.len == ESP_UUID_LEN_16 && descr_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                    ESP_LOGI(DEVICE_TAG, "Writing notify descr");
                    ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                 profiles[idx].conn_id,
                                                                 descr_result[0].handle,
                                                                 sizeof(notify_en),
                                                                 (uint8_t *)&notify_en,
                                                                 ESP_GATT_WRITE_TYPE_RSP,
                                                                 ESP_GATT_AUTH_REQ_NONE);
                }

                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(DEVICE_TAG, "esp_ble_gattc_write_char_descr error");
                    disconnected_handler(idx);
                }

                free(descr_result);
            }
        }
        else {
            ESP_LOGE(DEVICE_TAG, "decsr not found");
            disconnected_handler(idx);
        }
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG, "write descr failed, error status = %x", p_data->write.status);
            disconnected_handler(idx);
            break;
        }
        ESP_LOGI(DEVICE_TAG, "write descr success");

        // this was the last step of the connection process !
        connection_success_handler(idx);


        break;
    case ESP_GATTC_NOTIFY_EVT:
        if (p_data->notify.is_notify && profiles[idx].data_callback) {
            profiles[idx].data_callback(idx, p_data->notify.value, p_data->notify.value_len);
        }
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;

        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));

        ESP_LOGI(DEVICE_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
                 (bda[4] << 8) + bda[5]);

        bool ret = update_device_bda(idx, bda);
        if (!ret) {
            ESP_LOGE(DEVICE_TAG, "Failed to update device bda");
        }
            
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT:
        if (memcmp(p_data->disconnect.remote_bda, profiles[idx].remote_bda, 6) == 0){
            disconnected_handler(idx);
        }
        break;
    default:
        break;
    }
    }
}

void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{

    switch (event) {
        case ESP_GATTC_REG_EVT:
            if (param->reg.status == ESP_GATT_OK) {
            profiles[param->reg.app_id].gattc_if = gattc_if;
            } else {
                ESP_LOGI(GATTC_TAG, "Reg app failed, app_id %04x, status %d",
                        param->reg.app_id,
                        param->reg.status);
                return;
            }
            break;
        case ESP_GATTC_UNREG_EVT:
            ESP_LOGI(GATTC_TAG, "Profile unregistered");
            break;
        default:
            break;
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < MAX_DEVICES; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == profiles[idx].gattc_if) {
                gattc_profile_callback(event, gattc_if, param);
            }
        }
    } while (0);
}

bool init_gattc() {
    //register the callback function to the gattc module
    esp_err_t ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "gattc register error, error code = %x", ret);
        return false;
    }

    // set local MTU
    ret = esp_ble_gatt_set_local_mtu(200);
    if (ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", ret);
        return false;
    }    

    ESP_LOGI(GATTC_TAG, "GATTC initialized");
    return true;

}

// if called with idx = -1, will try to find next available profile
void open_profile(esp_bd_addr_t bda, esp_ble_addr_type_t ble_addr_type, size_t idx, device_type_t type) {

    if (idx != -1 && !is_profile_available(idx)) {
        ESP_LOGE(GATTC_TAG, "Trying to open profile at idx %d, but profile is already in use", idx);
        return;
    }

    if (idx == -1) {
        idx = next_available_profile_idx();
    }

    if (idx == -1) {
        ESP_LOGE(GATTC_TAG, "Trying to open a new profile, but no available profile found");
        return;
    }

    ESP_LOGI(GATTC_TAG, "Registering device at idx %d", idx);
    connection_start_handler(idx);

    device_type_config_t device_config = get_device_config(type);

    profiles[idx].ble_addr_type = ble_addr_type;
    profiles[idx].device_type = type;
    profiles[idx].data_callback = device_config.data_callback;
    memcpy(profiles[idx].remote_bda, bda, 6);

    esp_err_t ret = esp_ble_gattc_app_register(idx);
    if (ret){
        ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
        connection_end_handler(idx);
        return;
    }

}

bool is_profile_active(size_t idx) {
    return profiles[idx].gattc_if != ESP_GATT_IF_NONE;
}