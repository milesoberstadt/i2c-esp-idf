#include "i2c_messages.h"

#define HEADER_LEN 4

void process_message(uint8_t* data, size_t length) {
    if (data == NULL || length == 0) {
        ESP_LOGE(I2C_MESSAGES_TAG, "Invalid data received");
        return;
    }

    // Skip message processing when the first 4 bytes are all zeros
    if (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0) {
        ESP_LOGD(I2C_MESSAGES_TAG, "Skipping empty message (first 4 bytes are zero)");
        return;
    }

    // First, log the raw message for debugging
    ESP_LOGI(I2C_MESSAGES_TAG, "Raw message received:");
    ESP_LOG_BUFFER_HEX(I2C_MESSAGES_TAG, data, (length < 16) ? length : 16);

    uint8_t msg_type = data[0];
    uint8_t dev_idx = data[1];
    uint8_t msg_len = data[2];
    uint8_t identity = data[3];

    ESP_LOGI(I2C_MESSAGES_TAG, "Message received - type: %d, device: %d, length: %d, identity: 0x%02X", 
             msg_type, dev_idx, msg_len, identity);

    // Special case for identifier request - this message doesn't need identity verification
    // since it's the first message we would receive before the dom node knows our identifier
    if (msg_type == msg_req_identifier) {
        ESP_LOGI(I2C_MESSAGES_TAG, "Identified an identifier request message");
        // Prepare identifier response (1 byte for identifier)
        uint8_t id_data[1] = {device_identifier};
        // Send response back to master
        i2c_send_message_data(msg_res_identifier, 0, id_data, 1);
        return;
    }
    
    // For all other messages after identification, verify the identifier
    // If the message doesn't contain our identifier, ignore it
    // This allows multiple sub nodes to filter messages based on the identity byte
    if (identity != 0 && identity != device_identifier) {
        ESP_LOGW(I2C_MESSAGES_TAG, "Message identity 0x%02X doesn't match this device 0x%02X, ignoring", 
                identity, device_identifier);
        return;
    }

    // Handle devices message
    switch (msg_type) {
        case msg_init_start:
            ESP_LOGI(I2C_MESSAGES_TAG, "Initialization started");
            break;
            
        case msg_init_end:
            ESP_LOGI(I2C_MESSAGES_TAG, "Initialization ended");
            break;
            
        case msg_data:
            ESP_LOGI(I2C_MESSAGES_TAG, "Data received from dom node");
            if (msg_len > 0) {
                ESP_LOGI(I2C_MESSAGES_TAG, "Received data:");
                ESP_LOG_BUFFER_HEX_LEVEL(I2C_MESSAGES_TAG, data + HEADER_LEN, msg_len, ESP_LOG_DEBUG);
            }
            break;
            
        case msg_req_data:
            ESP_LOGI(I2C_MESSAGES_TAG, "Data request received from dom node");
            
            // Generate dummy response data
            uint8_t dummy_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
            
            // Send response back to master
            i2c_send_message_data(msg_res_data, dev_idx, dummy_data, 4);
            break;
            
        case msg_set_i2c_address:
            ESP_LOGI(I2C_MESSAGES_TAG, "Identified an I2C address reassignment message");
            if (msg_len >= 1) { // Need at least the address data
                uint8_t new_i2c_address = data[HEADER_LEN];
                
                ESP_LOGI(I2C_MESSAGES_TAG, "Received I2C address reassignment: 0x%02X", new_i2c_address);
                
                // First print a more verbose message about current status
                ESP_LOGI(I2C_MESSAGES_TAG, "Preparing to change I2C address from 0x%02X to 0x%02X", 
                         i2c_slave_addr, new_i2c_address);
                
                // Add a short delay before sending acknowledgment
                vTaskDelay(pdMS_TO_TICKS(50));
                
                // Send acknowledgment back to the master before changing the address
                ESP_LOGI(I2C_MESSAGES_TAG, "Sending acknowledgment before changing I2C address");
                uint8_t ack_data[1] = {new_i2c_address};
                i2c_send_message_data(msg_res_data, dev_idx, ack_data, 1);
                
                // Longer delay to allow the master to receive the response and be ready
                // for the slave device to disappear from its current address
                vTaskDelay(pdMS_TO_TICKS(200));
                
                // Change the I2C address
                // This will delete the current slave device and create a new one with the new address
                if (i2c_slave_change_address(new_i2c_address)) {
                    ESP_LOGI(I2C_MESSAGES_TAG, "Address saved to NVS. Rebooting in 300 ms to apply changes...");
                    vTaskDelay(pdMS_TO_TICKS(300));
                    esp_restart();
                } else {
                    ESP_LOGE(I2C_MESSAGES_TAG, "Failed to change I2C address to 0x%02X", new_i2c_address);
                }
            } else {
                ESP_LOGW(I2C_MESSAGES_TAG, "Received I2C address reassignment message with insufficient data");
            }
            break;

        case msg_set_wifi_channel:
            ESP_LOGI(I2C_MESSAGES_TAG, "Identified a WiFi channel assignment message");
            if (msg_len >= 1) { // Need at least the channel data
                uint8_t wifi_channel = data[HEADER_LEN];
                
                ESP_LOGI(I2C_MESSAGES_TAG, "Received WiFi channel assignment: %d", wifi_channel);
                
                // Here you would configure the WiFi channel for this device
                // For now we just log it
                ESP_LOGI(I2C_MESSAGES_TAG, "WiFi channel %d assigned to sub node (ID: 0x%02X)", 
                         wifi_channel, device_identifier);
            } else {
                ESP_LOGW(I2C_MESSAGES_TAG, "Received WiFi channel message with insufficient data");
            }
            break;
            
        default:
            ESP_LOGI(I2C_MESSAGES_TAG, "Unknown message type received: %d", msg_type);
            break;
    }
}

void i2c_send_message_data(message_t msg, uint8_t dev_idx, uint8_t *data, size_t data_len) {
    if (data_len > I2C_DATA_LEN - HEADER_LEN) {
        ESP_LOGE(I2C_MESSAGES_TAG, "Data length exceeds maximum allowed length");
        return;
    }

    uint8_t *msg_data = (uint8_t *)malloc(I2C_DATA_LEN);

    if (msg_data == NULL) {
        ESP_LOGE(I2C_MESSAGES_TAG, "Failed to allocate memory for message data");
        return;
    }

    // Clear buffer completely before building the message
    memset(msg_data, 0, I2C_DATA_LEN);

    // Build the message header
    msg_data[0] = msg;
    msg_data[1] = dev_idx;
    msg_data[2] = data_len;
    msg_data[3] = device_identifier;

    if (data != NULL && data_len > 0) {
        memcpy(msg_data + HEADER_LEN, data, data_len);
    }

    // Fill rest with padding
    for (size_t i = HEADER_LEN + data_len; i < I2C_DATA_LEN; i++) {
        msg_data[i] = 0xFF;
    }

    // Send the message
    ESP_LOGI(I2C_MESSAGES_TAG, "Sending message type: %d (0x%02X), length: %d, identity: 0x%02X",
            msg, msg, data_len, msg_data[3]);
    ESP_LOG_BUFFER_HEX(I2C_MESSAGES_TAG, msg_data, I2C_DATA_LEN);

    i2c_slave_send(msg_data, I2C_DATA_LEN);

    // Free allocated memory
    free(msg_data);
}

void i2c_send_message(message_t msg, uint8_t dev_idx) {
    i2c_send_message_data(msg, dev_idx, NULL, 0);
}