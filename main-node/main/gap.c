#include "gap.h"

static bool is_scanning = false;
static size_t scann_idx;

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_ENABLE
};

void start_scan(void)
{
    esp_ble_gap_start_scanning(PAIRING_DURATION);
}

void scan_started_handler() {
    if (is_scanning) {
        return;
    }
    is_scanning = true;
    scann_idx = get_selected_device();
    on_pairing_start(scann_idx);
}

void scan_ended_handler() {
    if (!is_scanning) {
        return;
    }
    is_scanning = false;
    on_pariring_stop(scann_idx);
}

void handle_scan_result(esp_ble_gap_cb_param_t *scan_result) {

    switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:

            // esp_log_buffer_hex(GAP_TAG, scan_result->scan_rst.bda, 6);
            // ESP_LOGI(GAP_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);

            uint8_t service_uuid[ESP_UUID_LEN_128];
            if (get_adv_service_uuid(
                scan_result->scan_rst.ble_adv, 
                scan_result->scan_rst.adv_data_len, 
                service_uuid)) {

                ESP_LOGI(GAP_TAG, "Service UUID found in advertisement data:");
                esp_log_buffer_hex(GAP_TAG, service_uuid, ESP_UUID_LEN_128);

                device_type_t type = get_device_type_from_uuid(service_uuid);

                if (type == UNKNOWN_DEVICE) {
                    ESP_LOGI(GAP_TAG, "Service UUID not matching with any device type UUID.");
                    // todo : maybe stop analysing this device
                    break;
                }
                
                ESP_LOGI(GAP_TAG, "Device service uuid matching with device type %d", type);

                // service uuid is matching, start gattc
                esp_ble_gap_stop_scanning();

                open_profile(scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, scann_idx, type);

            } 

            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            ESP_LOGI(GAP_TAG, "SCAN COMPLETED");
            scan_ended_handler();
            break;
        default:
            break;
    }
}

void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:

        // connnection update event log 
         ESP_LOGI(GAP_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);

        break;

    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        // scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(GAP_TAG, "SCAN STARTED SUCCESSFULLY");
            scan_started_handler();
        }else{
            ESP_LOGE(GAP_TAG, "Scan start failed");
        }
        break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT: {

        if (!is_scanning) {
            break;
        }

        // one scan result has came
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        handle_scan_result(scan_result);
        
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GAP_TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(GAP_TAG, "SCAN STOPPED SUCCESSFULLY");
        scan_ended_handler();

        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GAP_TAG, "Adv stop failed");
            break;
        }
        ESP_LOGI(GAP_TAG, "ADV STOPPED SUCCESSFULLY");
        break;
    default:
        break;
    }
}

bool init_gap() {
    // register the  callback function to the gap module
    esp_err_t ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret){
        ESP_LOGE(GAP_TAG, "gap register error, error code = %x", ret);
        return false;
    }

    // set scan parameters
    ret = esp_ble_gap_set_scan_params(&ble_scan_params);
    if (ret){
        ESP_LOGE(GAP_TAG, "set scan params error, error code = %x", ret);
        return false;
    }

    ESP_LOGI(GAP_TAG, "GAP initialized");
    return true;
}

bool get_is_scanning() {
    return is_scanning;
}