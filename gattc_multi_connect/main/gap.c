#include "gap.h"

// static const char remote_device_name[3][20] = {"ESP_GATTS_DEMO_a", "ESP_GATTS_DEMO_b", "ESP_GATTS_DEMO_c"};

static bool Isconnecting    = false;
static bool stop_scan_done  = false;

// static bool conn_device_a   = false;
// static bool conn_device_b   = false;
// static bool conn_device_c   = false;

void start_scan(void)
{
    stop_scan_done = false;
    Isconnecting = false;
    uint32_t duration = 30;
    esp_ble_gap_start_scanning(duration);
}

void handle_scan_result(esp_ble_gap_cb_param_t *scan_result) {

    switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:

            // device found
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(GATTC_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);

            uint8_t service_uuid[16];
            if (get_adv_service_uuid(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len, service_uuid)) {
                uint8_t target_uuid[16];
                uuid_str_to_bytes(REMOTE_SERVICE_UUID, target_uuid);
                
                if (!compare_uuid(service_uuid, target_uuid)) {
                    // this is not the service that we are looking for
                    // todo : stop scanning this device
                    break;
                }

                ESP_LOGI(GATTC_TAG, "Service UUID found !!!!.");

            } else {
                ESP_LOGI(GATTC_TAG, "Service UUID not found in advertisement data.");
            }

            ESP_LOGI(GATTC_TAG, "--------");

            return;

            if (Isconnecting){
                break;
            }
            // if (conn_device_a && conn_device_b && conn_device_c && !stop_scan_done){
            //     stop_scan_done = true;
            //     esp_ble_gap_stop_scanning();
            //     ESP_LOGI(GATTC_TAG, "all devices are connected");
            //     break;
            // }
            // if (adv_service_uuid != NULL) {

            //     if (strlen(remote_device_name[0]) == adv_service_uuid && strncmp((char *)adv_service_uuid, remote_device_name[0], adv_service_uuid_len) == 0) {
            //         if (conn_device_a == false) {
            //             conn_device_a = true;
            //             ESP_LOGI(GATTC_TAG, "Searched device %s", remote_device_name[0]);
            //             esp_ble_gap_stop_scanning();
            //             // esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
            //             Isconnecting = true;
            //         }
            //         break;
            //     }
            //     else if (strlen(remote_device_name[1]) == adv_service_uuid_len && strncmp((char *)adv_service_uuid, remote_device_name[1], adv_service_uuid_len) == 0) {
            //         if (conn_device_b == false) {
            //             conn_device_b = true;
            //             ESP_LOGI(GATTC_TAG, "Searched device %s", remote_device_name[1]);
            //             esp_ble_gap_stop_scanning();
            //             // esp_ble_gattc_open(gl_profile_tab[PROFILE_B_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
            //             Isconnecting = true;

            //         }
            //     }
            //     else if (strlen(remote_device_name[2]) == adv_service_uuid_len && strncmp((char *)adv_service_uuid, remote_device_name[2], adv_service_uuid_len) == 0) {
            //         if (conn_device_c == false) {
            //             conn_device_c = true;
            //             ESP_LOGI(GATTC_TAG, "Searched device %s", remote_device_name[2]);
            //             esp_ble_gap_stop_scanning();
            //             // esp_ble_gattc_open(gl_profile_tab[PROFILE_C_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
            //             Isconnecting = true;
            //         }
            //         break;
            //     }

            // }

            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            // scan finished
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
         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);

        break;

    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {

        // SCAN PARAMETERS ARE SET, START SCANNING
        uint32_t duration = 30;         // seconds
        esp_ble_gap_start_scanning(duration);
        break;
        
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:

        // scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(GATTC_TAG, "Scan start success");
        }else{
            ESP_LOGE(GATTC_TAG, "Scan start failed");
        }

        break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT: {

        // one scan result has came
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;

        handle_scan_result(scan_result);
        
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop scan successfully");

        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Adv stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop adv successfully");
        break;

    default:
        break;
    }
}

void init_gap() {
    //register the  callback function to the gap module
    esp_err_t ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret){
        ESP_LOGE(GATTC_TAG, "gap register error, error code = %x", ret);
        return;
    }
}
