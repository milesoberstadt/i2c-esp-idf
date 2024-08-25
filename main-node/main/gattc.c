#include "gattc.h"


// store connected devices info
static gattc_profile_inst profiles[MAX_DEVICES];

// constant notify descriptor uuid
static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

bool is_profile_active(size_t idx) {
    if (idx >= MAX_DEVICES) {
        return false;
    }
    return profiles[idx].gattc_if != ESP_GATT_IF_NONE;
}

size_t next_available_profile_idx() {
    for (size_t i = 0; i < MAX_DEVICES; i++) {
        if (is_profile_active(i)) {
            return i;
        }
    }
    return -1;
}

// used to generate device tag, for logging purpose
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

size_t get_char_idx_by_handle(size_t idx, uint16_t handle) {
    device_type_config_t device_config = get_device_config(profiles[idx].device_type);
    for (size_t i = 0; i < device_config.char_count; i++) {
        if (profiles[idx].char_handles[i] == handle) {
            return i;
        }
    }
    return -1;
}

// call this function to disconnect from bluetooth & clear the profile
void disconnect(size_t idx) {

    if (!is_profile_active(idx)) {
        ESP_LOGE(GATTC_TAG, "Trying to disconnect profile at idx %d, but profile is not active", idx);
        return;
    }

    on_device_state_changed(idx, dev_state_disconnecting);
    
    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    if (profiles[idx].gattc_if != ESP_GATT_IF_NONE) {

        esp_err_t ret = esp_ble_gattc_close(profiles[idx].gattc_if, profiles[idx].conn_id);

        if (ret != ESP_OK) {
            ESP_LOGE(DEVICE_TAG, "gattc close, error code = %x", ret);
        }
        
        ret = esp_ble_gattc_app_unregister(profiles[idx].gattc_if);
        if (ret != ESP_OK){
            ESP_LOGE(DEVICE_TAG, "gattc app unregister error, error code = %x", ret);
            return;
        } 
        
        profiles[idx].app_id = ESP_GATT_IF_NONE;
        profiles[idx].conn_id = ESP_GATT_IF_NONE;
        profiles[idx].gattc_if = ESP_GATT_IF_NONE;

        ESP_LOGI(DEVICE_TAG, "cleared profile");
        
    }

    free(profiles[idx].char_handles);
    profiles[idx].connected = false;
    profiles[idx].discovered = false;
    profiles[idx].subscribe_count = 0;
    profiles[idx].device_type = UNKNOWN_DEVICE;

    on_device_state_changed(idx, dev_state_disconnected);

    ESP_LOGI(DEVICE_TAG, "DEVICE DISCONNECTED");

}

// this function is used to search for a service on a device
void search_service(size_t idx, esp_bt_uuid_t service_uuid) {

    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    esp_gatt_status_t status = esp_ble_gattc_search_service(profiles[idx].gattc_if,
                                                            profiles[idx].conn_id,
                                                            &service_uuid);

    if (status != ESP_GATT_OK){
        ESP_LOGE(DEVICE_TAG, "esp_ble_gattc_search_service error, error status = %x", status);
        disconnect(idx);
    }

}

// when a characteristic notification subscription is successful, this is called
void subscription_success_handler(size_t idx) {

    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    if (!is_profile_active(idx)) {
        ESP_LOGE(DEVICE_TAG, "Trying to handle connection success for profile at idx %d, but profile is not active", idx);
        return;
    }

    // we want to check if every characteristic has been subscribed to, so we count it.
    profiles[idx].subscribe_count++;
    device_type_config_t device_config = get_device_config(profiles[idx].device_type);

    // if some of them are missing, we wait for them
    if (profiles[idx].subscribe_count != device_config.char_count) {
        return;
    }
    
    // if we reach this code, we have successfully subscribed to all characteristics
    // we can consider the device as connected
    on_device_state_changed(idx, dev_state_connected);

    // save the device in memory to be able to reconnect to it later
    bool ret = add_device(profiles[idx].remote_bda, profiles[idx].ble_addr_type, profiles[idx].device_type, idx);
    if (!ret) {
        ESP_LOGE(DEVICE_TAG, "Failed to save device");
    }

    // we also want to look for the battery characteristic
    ESP_LOGI(DEVICE_TAG, "All characteristics subscribed ! Looking for battery service ...");
    search_service(idx, UUID_BATTERY_SERVICE);

}

// this function is called when a gattc connection is opened
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

// we use this to get the characteristic count of a service
uint16_t get_characteristic_count(size_t idx, esp_gatt_db_attr_type_t type, uint16_t start_handle, uint16_t end_handle, uint16_t char_handle) {

    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    uint16_t count = 0;
    esp_gatt_status_t status = esp_ble_gattc_get_attr_count(profiles[idx].gattc_if,
                                                            profiles[idx].conn_id,
                                                            type,
                                                            start_handle,
                                                            end_handle,
                                                            char_handle,
                                                            &count);
    if (status != ESP_GATT_OK){
        ESP_LOGE(DEVICE_TAG, "esp_ble_gattc_get_attr_count error");
        disconnect(idx);
        return 0;
    }

    return count;

}

// this function is used to discover characteristics of a service and subscribe to them
size_t discover_characteristics(size_t idx, size_t count) {
    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    esp_gattc_char_elem_t *char_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);

    if (!char_result){
        ESP_LOGE(DEVICE_TAG, "gattc no mem");
        disconnect(idx);
        return 0;
    } 

    size_t found_char_count = 0;


    // get the device configuration
    device_type_config_t device_config = get_device_config(profiles[idx].device_type);

    // we loop through all characteristics that are expected
    for (size_t i = 0; i < device_config.char_count; i++) {

        esp_bt_uuid_t char_uuid = device_config.char_uuids[i];

        ESP_LOGI(DEVICE_TAG, "Looking for characteristic %d", i);
        esp_log_buffer_hex(DEVICE_TAG, (void *)&char_uuid, sizeof(esp_bt_uuid_t));
        
        uint16_t char_count;

        // we get the characteristic by its UUID
        esp_gatt_status_t status = esp_ble_gattc_get_char_by_uuid(profiles[idx].gattc_if,
                                            profiles[idx].conn_id,
                                            profiles[idx].service_start_handle,
                                            profiles[idx].service_end_handle,
                                            char_uuid,
                                            char_result,
                                            &char_count);

        if (status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG, "esp_ble_gattc_get_char_by_uuid error, error status = %x", status);
            continue;
        }

        ESP_LOGI(DEVICE_TAG, "Characteristic found, char_handle %d char property %x", char_result[0].char_handle, char_result[0].properties);

        // if we found it, we check if it has the notify property
        if (char_count > 0 && (char_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){

            ESP_LOGI(DEVICE_TAG, "char property has notify");

            profiles[idx].char_handles[i] = char_result[0].char_handle;

            // we register for notifications
            esp_ble_gattc_register_for_notify(  profiles[idx].gattc_if, 
                                                profiles[idx].remote_bda, 
                                                char_result[0].char_handle);

            found_char_count++;

        } else {
            ESP_LOGE(DEVICE_TAG, "no char property has notify");
            continue;
        }

    }
    
    free(char_result);
    char_result = NULL;

    return found_char_count;
}

// this function is called when the main service of a device is found
void main_service_found_handler(size_t idx) {

    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    device_type_config_t device_config = get_device_config(profiles[idx].device_type);
    
    ESP_LOGI(DEVICE_TAG, "Service found, looking for characteristics ...");

    // try looking for the device characteristics
    uint16_t count = get_characteristic_count(  idx, 
                                                ESP_GATT_DB_CHARACTERISTIC, 
                                                profiles[idx].service_start_handle,
                                                profiles[idx].service_end_handle,
                                                INVALID_HANDLE);
    
    ESP_LOGI(DEVICE_TAG, "Characteritics found : %d", count);

    if (count == 0) {
        ESP_LOGE(DEVICE_TAG, "0 char detected on the service, disconnecting ...");
        disconnect(idx);
        return;
    }

    ESP_LOGI(DEVICE_TAG, "Getting characteristics ...");
    size_t found_char = discover_characteristics(idx, count);

    if (found_char < device_config.char_count) {
        ESP_LOGE(DEVICE_TAG, "Only %d out of %d char were found, disconnecting...", found_char, device_config.char_count);
        disconnect(idx);
        return;
    }

}

// this function is called when the battery service of a device is found
void battery_service_found_handler(size_t idx) {

    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    ESP_LOGI(DEVICE_TAG, "Battery service found, looking for battery level characteristic ...");

    uint16_t count = get_characteristic_count(  idx, 
                                    ESP_GATT_DB_CHARACTERISTIC, 
                                    profiles[idx].battery_service_start_handle,
                                    profiles[idx].battery_service_end_handle,
                                    INVALID_HANDLE);

    esp_gattc_char_elem_t *char_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);

    if (!char_result){
        ESP_LOGE(DEVICE_TAG, "gattc no mem");
        disconnect(idx);
        return;
    } 

    esp_gatt_status_t status = esp_ble_gattc_get_char_by_uuid(profiles[idx].gattc_if,
                                        profiles[idx].conn_id,
                                        profiles[idx].battery_service_start_handle,
                                        profiles[idx].battery_service_end_handle,
                                        UUID_BATTERY_CHARACTERISTIC,
                                        char_result,
                                        &count);

    if (status != ESP_GATT_OK){
        ESP_LOGE(DEVICE_TAG, "esp_ble_gattc_get_char_by_uuid error, error status = %x", status);
        disconnect(idx);
        free(char_result);
        return;
    }

    if (count > 0 && (char_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){

        ESP_LOGI(DEVICE_TAG, "char property has notify");

        profiles[idx].battery_char_handle = char_result[0].char_handle;

        esp_ble_gattc_register_for_notify(  profiles[idx].gattc_if, 
                                            profiles[idx].remote_bda, 
                                            char_result[0].char_handle);

    } else {
        ESP_LOGE(DEVICE_TAG, "no char property has notify");
        disconnect(idx);
        free(char_result);
        return;
    }

    free(char_result);
    char_result = NULL;
}

// this function is called when a gattc event is triggered for a specific profile (=device)
void gattc_profile_callback(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{

    // we have to find the profile index by the gattc interface
    size_t idx = get_idx_by_gattc_if(gattc_if);

    if (idx == -1) {
        ESP_LOGE(GATTC_TAG, "Profile not found for gattc_if %d", gattc_if);
        return;
    }

    // we generate a tag for logging
    char DEVICE_TAG[DEVICE_TAG_SIZE];
    generate_device_tag(idx, DEVICE_TAG);

    // we get the configuration of the device
    device_type_config_t device_config = get_device_config(profiles[idx].device_type);

    // and event parameters
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
            disconnect(idx);
            break;
        } 

        connection_oppened_handler(idx, gattc_if, p_data);

        break;
    case ESP_GATTC_CFG_MTU_EVT:

        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG,"Config mtu failed");
            disconnect(idx);
            return;
        }

        ESP_LOGI(DEVICE_TAG, "mut set : Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        ESP_LOGI(DEVICE_TAG, "Looking for services...");

        search_service(idx, device_config.service_uuid);

        break;
    // when a search result is found (service search)
    case ESP_GATTC_SEARCH_RES_EVT: {

        // if the service found is the device main service
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_128 && compare_uuid(device_config.service_uuid.uuid.uuid128, p_data->search_res.srvc_id.uuid.uuid.uuid128)) {

            // we store the service information
            profiles[idx].service_start_handle = p_data->search_res.start_handle;
            profiles[idx].service_end_handle = p_data->search_res.end_handle;
            profiles[idx].discovered = true;

            main_service_found_handler(idx);
           
            break;
        } 

        // if found service is the battery service
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_128 && compare_uuid(UUID_BATTERY_SERVICE.uuid.uuid128, p_data->search_res.srvc_id.uuid.uuid.uuid128)) {

            profiles[idx].battery_service_start_handle = p_data->search_res.start_handle;
            profiles[idx].battery_service_end_handle = p_data->search_res.end_handle;

            battery_service_found_handler(idx);

            break;
        }

        ESP_LOGW(DEVICE_TAG, "Service search result found, but UUID don't match");

        break;
    }
    // when the service search is completed
    case ESP_GATTC_SEARCH_CMPL_EVT:
        ESP_LOGI(DEVICE_TAG, "Service search completed: conn_id = %x, status %d", p_data->search_cmpl.conn_id, p_data->search_cmpl.status);

        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG, "search service failed, error status = %d", p_data->search_cmpl.status);
            disconnect(idx);
            break;
        }
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        uint16_t char_handle = p_data->reg_for_notify.handle;
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG, "Failed to register for notifications for char handle %d, error status =%x, disconnecting...", char_handle, p_data->reg_for_notify.status);
            disconnect(idx);
            break;
        }

        ESP_LOGI(DEVICE_TAG, "Registering for notifications for char idx %d ...", char_handle);

        uint16_t count;

        // count characteristic descriptors for the correct service
        if (char_handle == profiles[idx].battery_char_handle) {
            count = get_characteristic_count(  idx, 
                                                ESP_GATT_DB_DESCRIPTOR,
                                                profiles[idx].battery_service_start_handle,
                                                profiles[idx].battery_service_end_handle,
                                                char_handle);
        } else {
            count = get_characteristic_count(  idx, 
                                    ESP_GATT_DB_DESCRIPTOR,
                                    profiles[idx].service_start_handle,
                                    profiles[idx].service_end_handle,
                                    char_handle);
        }

        uint16_t notify_en = 1;

        if (count > 0){

            esp_gattc_descr_elem_t* descr_result = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t) * count);
            if (!descr_result){
                ESP_LOGE(DEVICE_TAG, "malloc error, gattc no mem");
                disconnect(idx);
            } else {
                ESP_LOGI(DEVICE_TAG, "Getting attribute descriptor %d", count);
                esp_gatt_status_t ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                     profiles[idx].conn_id,
                                                                     char_handle,
                                                                     notify_descr_uuid,
                                                                     descr_result,
                                                                     &count);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(DEVICE_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                    disconnect(idx);
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
                    disconnect(idx);
                }

                free(descr_result);
            }
        }
        else {
            ESP_LOGE(DEVICE_TAG, "decsr not found");
            disconnect(idx);
        }
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(DEVICE_TAG, "write descr failed, error status = %x", p_data->write.status);
            disconnect(idx);
            break;
        }
        ESP_LOGI(DEVICE_TAG, "write descr success");

        // this was the last step of the connection process !
        subscription_success_handler(idx);

        break;
    case ESP_GATTC_NOTIFY_EVT:
        if (p_data->notify.is_notify) {

            // handle battery service
            if (p_data->notify.handle == profiles[idx].battery_char_handle) {
                on_battery_level_received(idx, p_data->notify.value[0]);
                break;;
            }

            // handle regular data service notification
            size_t char_idx = get_char_idx_by_handle(idx, p_data->notify.handle);

            if (char_idx == -1) {
                ESP_LOGE(DEVICE_TAG, "Characteristic not found for handle %d", p_data->notify.handle);
                break;
            }

            on_data_received(   idx, 
                                char_idx, 
                                p_data->notify.value, 
                                p_data->notify.value_len);
                                        
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
            disconnect(idx);
        }
        break;
    default:
        break;
    }
    }
}

// this is the main callback function for the gattc module. it handle shared events, and dispatches device-specific ones to the right profile callback
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

    for (size_t idx = 0; idx < MAX_DEVICES; idx++) {
        if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                gattc_if == profiles[idx].gattc_if) {
            gattc_profile_callback(event, gattc_if, param);
        }
    }
}

bool init_gattc() {

    // init profiles interfaces
    for (size_t i = 0; i < MAX_DEVICES; i++) {
        profiles[i].gattc_if = ESP_GATT_IF_NONE;
    }

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

    if (idx != -1 && is_profile_active(idx)) {
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

    on_device_state_changed(idx, dev_state_connecting);
    on_device_type_changed(idx, type);

    device_type_config_t device_config = get_device_config(type);

    profiles[idx].ble_addr_type = ble_addr_type;
    profiles[idx].device_type = type;
    memcpy(profiles[idx].remote_bda, bda, 6);

    profiles[idx].char_handles = (uint16_t *)malloc(sizeof(uint16_t) * device_config.char_count);

    esp_err_t ret = esp_ble_gattc_app_register(idx);
    if (ret){
        ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
        disconnect(idx);
        return;
    }

}

