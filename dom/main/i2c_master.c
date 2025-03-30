#include "i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = I2C_MASTER_TAG;
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

esp_err_t i2c_master_init(void) {
    ESP_LOGI(TAG, "Initializing I2C master with the new driver API");
    ESP_LOGI(TAG, "I2C pins: SCL=%d, SDA=%d", I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO);
    ESP_LOGI(TAG, "I2C frequency: %d Hz", I2C_MASTER_FREQ_HZ);
    
    // Configure I2C master bus
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_PORT,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    // Create I2C master bus
    esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure I2C device
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_FIXED_SUB_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        // Note: ESP-IDF v5.4 doesn't support timeout in i2c_device_config_t
    };
    
    // Add device to the bus
    ret = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
        i2c_del_master_bus(bus_handle);
        return ret;
    }
    
    ESP_LOGI(TAG, "I2C master initialized successfully");
    ESP_LOGI(TAG, "Fixed SUB address: 0x%02X", I2C_FIXED_SUB_ADDR);
    return ESP_OK;
}

esp_err_t i2c_master_send(uint8_t addr, message_type_t msg_type, uint8_t sub_id, const uint8_t *data, uint8_t data_len) {
    if (data_len > I2C_MESSAGE_DATA_LEN) {
        ESP_LOGE(TAG, "Data too large for I2C message (%u > %u)", 
                 data_len, I2C_MESSAGE_DATA_LEN);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Prepare message buffer
    uint8_t buffer[I2C_DATA_LEN];
    memset(buffer, 0xFF, sizeof(buffer)); // Fill with 0xFF (unused marker)
    
    // Set up message header
    buffer[0] = (uint8_t)msg_type;   // Message type
    buffer[1] = sub_id;              // SUB ID
    buffer[2] = data_len;            // Data length
    
    // Copy data if provided
    if (data != NULL && data_len > 0) {
        memcpy(buffer + I2C_HEADER_LEN, data, data_len);
    }
    
    ESP_LOGI(TAG, "Sending I2C message: type=0x%02X, sub_id=0x%02X, data_len=%u to addr=0x%02X",
             msg_type, sub_id, data_len, addr);
    
    // Try multiple times
    int retries = 3;
    esp_err_t ret;
    
    while (retries-- > 0) {
        // Transmit data to the SUB
        ret = i2c_master_transmit(dev_handle, buffer, I2C_DATA_LEN, I2C_MASTER_TIMEOUT_MS);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Successfully sent message type 0x%02X to 0x%02X", msg_type, addr);
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "Failed to send I2C message to 0x%02X (retry %d): %s", 
                     addr, 2-retries, esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100)); // Short delay before retry
        }
    }
    
    return ret;
}

esp_err_t i2c_master_read(uint8_t addr, i2c_message_t *response) {
    if (!response) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Reading message from I2C device at address 0x%02X", addr);
    
    // Clear response buffer to help with debugging
    memset(response, 0, sizeof(i2c_message_t));
    
    // Try multiple times
    int retries = 3;
    esp_err_t ret;
    
    while (retries-- > 0) {
        // Read data from the SUB
        ret = i2c_master_receive(dev_handle, (uint8_t *)response, sizeof(i2c_message_t), I2C_MASTER_TIMEOUT_MS);
        
        if (ret == ESP_OK) {
            // Dump first few bytes to help debug
            ESP_LOGI(TAG, "Successfully read from 0x%02X - Header: type=0x%02X, sub_id=0x%02X, data_len=%u",
                    addr, response->header.msg_type, response->header.sub_id, response->header.data_len);
            
            // Log first few bytes of data for debugging
            if (response->header.data_len > 0) {
                ESP_LOGI(TAG, "Data[0..3]: 0x%02X 0x%02X 0x%02X 0x%02X...",
                    response->data[0], 
                    response->header.data_len > 1 ? response->data[1] : 0,
                    response->header.data_len > 2 ? response->data[2] : 0,
                    response->header.data_len > 3 ? response->data[3] : 0);
            }
            
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "Failed to read I2C response from 0x%02X (retry %d): %s",
                    addr, 2-retries, esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100)); // Short delay before retry
        }
    }
    
    return ret;
}

// Alias for i2c_master_read for compatibility with existing code
esp_err_t i2c_master_read_msg(uint8_t addr, i2c_message_t *response) {
    // This is just a wrapper around i2c_master_read to maintain backward compatibility
    return i2c_master_read(addr, response);
}

esp_err_t i2c_master_send_hello(uint8_t addr) {
    ESP_LOGI(TAG, "Sending hello to fixed SUB at address 0x%02X", addr);
    return i2c_master_send(addr, MSG_HELLO, 0, NULL, 0);
}

esp_err_t i2c_master_send_verify(uint8_t addr, const char *id_str) {
    if (!id_str || strlen(id_str) < 2) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Sending verify '%s' to fixed SUB at address 0x%02X", id_str, addr);
    return i2c_master_send(addr, MSG_VERIFY, 0, (const uint8_t *)id_str, 2);
}

esp_err_t i2c_master_send_assign(uint8_t addr, uint8_t new_addr, uint8_t wifi_channel, uint8_t sub_id) {
    uint8_t data[3];
    data[0] = new_addr;       // New I2C address (though in fixed mode, this will be the same)
    data[1] = wifi_channel;   // WiFi channel to scan
    data[2] = sub_id;         // SUB ID assigned by DOM
    
    ESP_LOGI(TAG, "Assigning WiFi channel %u, SUB ID %u to fixed SUB at 0x%02X (new addr: 0x%02X)",
             wifi_channel, sub_id, addr, new_addr);
    return i2c_master_send(addr, MSG_ASSIGN, 0, data, sizeof(data));
}

esp_err_t i2c_master_set_time(uint8_t addr, uint8_t sub_id, uint32_t timestamp) {
    ESP_LOGI(TAG, "Setting time %lu on SUB %u (0x%02X)", (unsigned long)timestamp, sub_id, addr);
    return i2c_master_send(addr, MSG_SET_TIME, sub_id, (const uint8_t *)&timestamp, sizeof(timestamp));
}

esp_err_t i2c_master_start_scan(uint8_t addr, uint8_t sub_id) {
    ESP_LOGI(TAG, "Starting scan on SUB %u (0x%02X)", sub_id, addr);
    return i2c_master_send(addr, MSG_START_SCAN, sub_id, NULL, 0);
}

esp_err_t i2c_master_stop_scan(uint8_t addr, uint8_t sub_id) {
    ESP_LOGI(TAG, "Stopping scan on SUB %u (0x%02X)", sub_id, addr);
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
    ret = i2c_master_read(addr, &response);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Check response type
    if (response.header.msg_type != MSG_AP_COUNT) {
        ESP_LOGW(TAG, "Unexpected response type: 0x%02X", response.header.msg_type);
        return ESP_ERR_INVALID_STATE; // Using ESP_ERR_INVALID_STATE instead of ESP_ERR_INVALID_RESPONSE
    }
    
    // Extract count
    if (response.header.data_len >= sizeof(uint16_t)) {
        memcpy(count, response.data, sizeof(uint16_t));
        ESP_LOGI(TAG, "SUB %u reported %u APs", sub_id, *count);
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
    ret = i2c_master_read(addr, &response);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Check response type
    if (response.header.msg_type != MSG_AP_DATA) {
        ESP_LOGW(TAG, "Unexpected response type: 0x%02X", response.header.msg_type);
        return ESP_ERR_INVALID_STATE; // Using ESP_ERR_INVALID_STATE instead of ESP_ERR_INVALID_RESPONSE
    }
    
    // Check if we have data
    if (response.header.data_len > 0) {
        // Extract AP record
        if (response.header.data_len >= sizeof(ap_record_t)) {
            memcpy(record, response.data, sizeof(ap_record_t));
            *has_more = true;
            ESP_LOGI(TAG, "Received AP data from SUB %u: RSSI %d", 
                     sub_id, record->rssi);
        } else {
            ESP_LOGW(TAG, "Invalid AP data length: %u", response.header.data_len);
            *has_more = false;
            return ESP_ERR_INVALID_ARG; // Using ESP_ERR_INVALID_ARG instead of ESP_ERR_INVALID_SIZE
        }
    } else {
        // No more data
        *has_more = false;
        ESP_LOGI(TAG, "No more AP data from SUB %u", sub_id);
    }
    
    return ESP_OK;
}

esp_err_t i2c_master_confirm_ap(uint8_t addr, uint8_t sub_id, const uint8_t *bssid) {
    if (!bssid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Confirming AP receipt on SUB %u", sub_id);
    return i2c_master_send(addr, MSG_CONFIRM_AP, sub_id, bssid, 6);
}

esp_err_t i2c_master_send_reset(uint8_t addr) {
    ESP_LOGI(TAG, "Sending reset command to SUB at address 0x%02X", addr);
    return i2c_master_send(addr, MSG_RESET, 0, NULL, 0);
}