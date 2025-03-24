#include "i2c_master.h"
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "i2c_master";

// I2C pins
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ_HZ 100000  // 100kHz

// I2C timeouts
#define I2C_TIMEOUT_MS 100

esp_err_t i2c_master_init(void) {
    ESP_LOGI(TAG, "Initializing I2C master");
    
    // I2C configuration
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ
    };
    
    // Initialize I2C driver
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &i2c_config));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));
    
    ESP_LOGI(TAG, "I2C master initialized");
    return ESP_OK;
}

esp_err_t i2c_master_scan(uint8_t start_addr, uint8_t end_addr, uint8_t *found_addrs, uint8_t *num_found) {
    if (!found_addrs || !num_found || start_addr > end_addr) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *num_found = 0;
    
    ESP_LOGI(TAG, "Starting I2C scan from address 0x%02X to 0x%02X", start_addr, end_addr);
    
    for (uint8_t addr = start_addr; addr <= end_addr && *num_found < 255; addr++) {
        ESP_LOGD(TAG, "Probing I2C address 0x%02X (%u)", addr, addr);
        
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found I2C device at address 0x%02X", addr);
            found_addrs[(*num_found)++] = addr;
        } else {
            ESP_LOGD(TAG, "No device at address 0x%02X, error: %s", addr, esp_err_to_name(ret));
        }
    }
    
    ESP_LOGI(TAG, "I2C scan complete: found %u devices in range 0x%02X-0x%02X", 
             *num_found, start_addr, end_addr);
    
    // Log found devices
    if (*num_found > 0) {
        ESP_LOGI(TAG, "Found devices at addresses:");
        for (uint8_t i = 0; i < *num_found; i++) {
            ESP_LOGI(TAG, "  - 0x%02X (%u)", found_addrs[i], found_addrs[i]);
        }
    }
    
    return ESP_OK;
}

esp_err_t i2c_master_send(uint8_t addr, message_type_t msg_type, uint8_t sub_id, const uint8_t *data, uint8_t data_len) {
    if (data_len > (I2C_DATA_LEN - sizeof(message_header_t))) {
        ESP_LOGE(TAG, "Data too large for I2C message (%u > %u)", 
                 data_len, I2C_DATA_LEN - sizeof(message_header_t));
        return ESP_ERR_INVALID_ARG;
    }
    
    // Prepare message buffer
    uint8_t buffer[I2C_DATA_LEN];
    memset(buffer, 0xFF, sizeof(buffer)); // Fill with 0xFF (unused marker)
    
    // Set up message header
    message_header_t *header = (message_header_t *)buffer;
    header->msg_type = msg_type;
    header->sub_id = sub_id;
    header->data_len = data_len;
    
    // Copy data if provided
    if (data != NULL && data_len > 0) {
        memcpy(buffer + sizeof(message_header_t), data, data_len);
    }
    
    // Create I2C command
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, buffer, sizeof(buffer), true);
    i2c_master_stop(cmd);
    
    // Execute command
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send I2C message to 0x%02X: %s", 
                 addr, esp_err_to_name(ret));
    } else {
        ESP_LOGD(TAG, "Sent message type 0x%02X to 0x%02X", msg_type, addr);
    }
    
    return ret;
}

esp_err_t i2c_master_read_msg(uint8_t addr, i2c_message_t *response) {
    if (!response) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Preparing to read message from I2C device at address 0x%02X", addr);
    
    // Clear response buffer to help with debugging
    memset(response, 0, sizeof(i2c_message_t));
    
    // Create I2C command
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    
    // Read the whole message
    i2c_master_read(cmd, (uint8_t *)response, sizeof(i2c_message_t), I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    
    // Execute command with increased timeout (200ms instead of 100ms)
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(200));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read I2C response from 0x%02X: %s",
                 addr, esp_err_to_name(ret));
    } else {
        // Dump first few bytes to help debug
        ESP_LOGI(TAG, "Read from 0x%02X - Header: type=0x%02X, sub_id=0x%02X, data_len=%u",
                 addr, response->header.msg_type, response->header.sub_id, response->header.data_len);
        
        // Log first few bytes of data for debugging
        if (response->header.data_len > 0) {
            ESP_LOGI(TAG, "Data[0..3]: 0x%02X 0x%02X 0x%02X 0x%02X...",
                   response->data[0], 
                   response->header.data_len > 1 ? response->data[1] : 0,
                   response->header.data_len > 2 ? response->data[2] : 0,
                   response->header.data_len > 3 ? response->data[3] : 0);
        }
    }
    
    return ret;
}

esp_err_t i2c_master_send_hello(uint8_t addr) {
    ESP_LOGD(TAG, "Sending hello to address 0x%02X", addr);
    return i2c_master_send(addr, MSG_HELLO, 0, NULL, 0);
}

esp_err_t i2c_master_send_verify(uint8_t addr, const char *id_str) {
    if (!id_str || strlen(id_str) < 2) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Sending verify '%s' to address 0x%02X", id_str, addr);
    return i2c_master_send(addr, MSG_VERIFY, 0, (const uint8_t *)id_str, 2);
}

esp_err_t i2c_master_send_assign(uint8_t old_addr, uint8_t new_addr, uint8_t wifi_channel, uint8_t sub_id) {
    uint8_t data[3];
    data[0] = new_addr;
    data[1] = wifi_channel;
    data[2] = sub_id;
    
    ESP_LOGD(TAG, "Assigning address 0x%02X -> 0x%02X, WiFi channel %u, SUB ID %u",
             old_addr, new_addr, wifi_channel, sub_id);
    return i2c_master_send(old_addr, MSG_ASSIGN, 0, data, sizeof(data));
}

esp_err_t i2c_master_send_reset(uint8_t addr) {
    ESP_LOGD(TAG, "Sending reset to address 0x%02X", addr);
    return i2c_master_send(addr, MSG_RESET, 0, NULL, 0);
}

esp_err_t i2c_master_set_time(uint8_t addr, uint8_t sub_id, uint32_t timestamp) {
    ESP_LOGD(TAG, "Setting time %lu on SUB %u (0x%02X)", (unsigned long)timestamp, sub_id, addr);
    return i2c_master_send(addr, MSG_SET_TIME, sub_id, (const uint8_t *)&timestamp, sizeof(timestamp));
}

esp_err_t i2c_master_start_scan(uint8_t addr, uint8_t sub_id) {
    ESP_LOGD(TAG, "Starting scan on SUB %u (0x%02X)", sub_id, addr);
    return i2c_master_send(addr, MSG_START_SCAN, sub_id, NULL, 0);
}

esp_err_t i2c_master_stop_scan(uint8_t addr, uint8_t sub_id) {
    ESP_LOGD(TAG, "Stopping scan on SUB %u (0x%02X)", sub_id, addr);
    return i2c_master_send(addr, MSG_STOP_SCAN, sub_id, NULL, 0);
}

esp_err_t i2c_master_req_ap_count(uint8_t addr, uint8_t sub_id, uint16_t *count) {
    if (!count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret;
    i2c_message_t response;
    
    // Send request for AP count
    ret = i2c_master_send(addr, MSG_REQ_AP_COUNT, sub_id, NULL, 0);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Small delay to allow SUB to prepare response
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // Read response
    ret = i2c_master_read_msg(addr, &response);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Check response type
    if (response.header.msg_type != MSG_AP_COUNT) {
        ESP_LOGW(TAG, "Unexpected response type: 0x%02X", response.header.msg_type);
        return ESP_ERR_INVALID_RESPONSE;
    }
    
    // Extract count
    if (response.header.data_len >= sizeof(uint16_t)) {
        memcpy(count, response.data, sizeof(uint16_t));
        ESP_LOGD(TAG, "SUB %u reported %u APs", sub_id, *count);
    } else {
        *count = 0;
        ESP_LOGW(TAG, "Invalid AP count data length: %u", response.header.data_len);
    }
    
    return ESP_OK;
}

esp_err_t i2c_master_req_ap_data(uint8_t addr, uint8_t sub_id, ap_record_t *record, bool *has_more) {
    if (!record || !has_more) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret;
    i2c_message_t response;
    
    // Send request for AP data
    ret = i2c_master_send(addr, MSG_REQ_AP_DATA, sub_id, NULL, 0);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Small delay to allow SUB to prepare response
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // Read response
    ret = i2c_master_read_msg(addr, &response);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Check response type
    if (response.header.msg_type != MSG_AP_DATA) {
        ESP_LOGW(TAG, "Unexpected response type: 0x%02X", response.header.msg_type);
        return ESP_ERR_INVALID_RESPONSE;
    }
    
    // Check if we have data
    if (response.header.data_len > 0) {
        // Extract AP record
        if (response.header.data_len >= sizeof(ap_record_t)) {
            memcpy(record, response.data, sizeof(ap_record_t));
            *has_more = true;
            ESP_LOGD(TAG, "Received AP data from SUB %u: RSSI %d", 
                     sub_id, record->rssi);
        } else {
            ESP_LOGW(TAG, "Invalid AP data length: %u", response.header.data_len);
            *has_more = false;
            return ESP_ERR_INVALID_SIZE;
        }
    } else {
        // No more data
        *has_more = false;
        ESP_LOGD(TAG, "No more AP data from SUB %u", sub_id);
    }
    
    return ESP_OK;
}

esp_err_t i2c_master_confirm_ap(uint8_t addr, uint8_t sub_id, const uint8_t *bssid) {
    if (!bssid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Confirming AP receipt on SUB %u", sub_id);
    return i2c_master_send(addr, MSG_CONFIRM_AP, sub_id, bssid, 6);
}