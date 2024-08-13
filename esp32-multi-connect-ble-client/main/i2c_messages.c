#include "i2c_messages.h"

void i2c_send_message(message_t msg, size_t dev_idx) {
    i2c_send_message_data(msg, dev_idx, NULL, 0);
}

void i2c_send_message_data(message_t msg, size_t dev_idx, uint8_t *data, size_t data_len) {

    size_t message_header_len = 2;
    size_t message_len = data_len + message_header_len + 1; // Add 1 for end of file character

    uint8_t *msg_data = (uint8_t *)malloc(message_len * sizeof(uint8_t));

    if (msg_data == NULL) {
        ESP_LOGE(I2C_MSG_TAG, "Failed to allocate memory for message data");
        return;
    }

    msg_data[0] = msg;
    msg_data[1] = dev_idx;

    memcpy(msg_data + message_header_len, data, data_len);

    msg_data[message_len - 1] = '\0'; // Set end of file character

    i2c_master_write_slave(msg_data, message_len, I2C_WIFI_SLAVE_ADDR);

    ESP_LOGI(I2C_MSG_TAG, "Message sent: %d", msg);
    esp_log_buffer_hex(I2C_MSG_TAG, msg_data, message_len);

    free(msg_data);
}