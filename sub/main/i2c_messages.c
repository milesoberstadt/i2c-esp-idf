#include "i2c_messages.h"

#define HEADER_LEN 3

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
    // Log message details for other message types
    // ESP_LOGI(I2C_MESSAGES_TAG, "Message type: 0x%02X, bytes[0-3]: 0x%02X 0x%02X 0x%02X 0x%02X",
    //             data[0], data[0], data[1], data[2], data[3]);

    uint8_t msg_type = data[0];
    uint8_t dev_idx = data[1];
    uint8_t msg_len = data[2];

    // ESP_LOGI(I2C_MESSAGES_TAG, "Message received - type: %d, device: %d, length: %d", 
    //          msg_type, dev_idx, msg_len);

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
            
        case msg_req_identifier:
            ESP_LOGI(I2C_MESSAGES_TAG, "Identified an identifier request message");
            // TODO: I think this is being read correctly, but I should change this to be a single byte
            // Prepare identifier response (2 bytes for identifier)
            uint8_t id_data[2];
            id_data[0] = (device_identifier >> 8) & 0xFF; // High byte
            id_data[1] = device_identifier & 0xFF;        // Low byte
            // Send response back to master
            i2c_send_message_data(msg_res_identifier, 0, id_data, 2);
            break;

        case msg_set_wifi_channel:
            ESP_LOGI(I2C_MESSAGES_TAG, "Identified a WiFi channel assignment message");
            if (msg_len >= 1) {
                uint8_t wifi_channel = data[HEADER_LEN];
                ESP_LOGI(I2C_MESSAGES_TAG, "Received WiFi channel assignment: %d", wifi_channel);
                
                // Here you would configure the WiFi channel for this device
                // For now we just log it
                ESP_LOGI(I2C_MESSAGES_TAG, "WiFi channel %d assigned to sub node", wifi_channel);
            } else {
                ESP_LOGW(I2C_MESSAGES_TAG, "Received WiFi channel message with no channel data");
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

    if (data != NULL && data_len > 0) {
        memcpy(msg_data + HEADER_LEN, data, data_len);
    }

    // Fill rest with padding
    for (size_t i = HEADER_LEN + data_len; i < I2C_DATA_LEN; i++) {
        msg_data[i] = 0xFF;
    }

    // Send the message
    ESP_LOGI(I2C_MESSAGES_TAG, "Sending message type: %d (0x%02X), length: %d",
            msg, msg, data_len);
    ESP_LOG_BUFFER_HEX(I2C_MESSAGES_TAG, msg_data, I2C_DATA_LEN);

    i2c_slave_send(msg_data, I2C_DATA_LEN);

    // Free allocated memory
    free(msg_data);
}

void i2c_send_message(message_t msg, uint8_t dev_idx) {
    i2c_send_message_data(msg, dev_idx, NULL, 0);
}