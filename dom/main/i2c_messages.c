#include "i2c_messages.h"

#define HEADER_LEN 3

void i2c_send_message(message_t msg, uint8_t dev_idx) {
    i2c_send_message_data(msg, dev_idx, NULL, 0);
}

void i2c_send_message_data(message_t msg, uint8_t dev_idx, uint8_t *data, size_t data_len) {
    if (data_len > I2C_DATA_LEN - HEADER_LEN) {
        ESP_LOGE(I2C_MSG_TAG, "Data length exceeds maximum allowed length");
        return;
    }

    uint8_t *msg_data = (uint8_t *)malloc(I2C_DATA_LEN);

    if (msg_data == NULL) {
        ESP_LOGE(I2C_MSG_TAG, "Failed to allocate memory for message data");
        return;
    }

    msg_data[0] = msg;
    msg_data[1] = dev_idx;
    msg_data[2] = data_len;

    if (data != NULL && data_len > 0) {
        memcpy(msg_data + HEADER_LEN, data, data_len);
    }

    for (size_t i = HEADER_LEN + data_len; i < I2C_DATA_LEN; i++) {
        msg_data[i] = 0xFF;
    }

    i2c_write(msg_data);

    ESP_LOGI(I2C_MSG_TAG, "Message sent: %d", msg);
    esp_log_buffer_hex(I2C_MSG_TAG, msg_data, I2C_DATA_LEN);

    free(msg_data);
}

void process_message(uint8_t* data, size_t length) {
    if (data == NULL || length == 0) {
        ESP_LOGE(I2C_MSG_TAG, "Invalid data received");
        return;
    }

    esp_log_buffer_hex(I2C_MSG_TAG, data, length);

    uint8_t msg_type = data[0];
    uint8_t dev_idx = data[1];
    uint8_t msg_len = data[2];

    ESP_LOGI(I2C_MSG_TAG, "Message received - type: %d, device: %d, length: %d", 
             msg_type, dev_idx, msg_len);

    // Handle message based on type
    switch (msg_type) {
        case msg_res_data:
            ESP_LOGI(I2C_MSG_TAG, "Data response received from sub node");
            if (msg_len > 0) {
                ESP_LOGI(I2C_MSG_TAG, "Received data: ");
                esp_log_buffer_hex(I2C_MSG_TAG, data + HEADER_LEN, msg_len);
            }
            break;
        
        default:
            ESP_LOGI(I2C_MSG_TAG, "Unknown message type received: %d", msg_type);
            break;
    }
}