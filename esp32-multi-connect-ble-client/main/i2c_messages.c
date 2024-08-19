#include "i2c_messages.h"

#define HEADER_LEN 3

void i2c_send_message(message_t msg, size_t dev_idx) {
    i2c_send_message_data(msg, dev_idx, NULL, 0);
}

void i2c_send_message_data(message_t msg, size_t dev_idx, uint8_t *data, size_t data_len) {

    uint8_t *msg_data = (uint8_t *)malloc(I2C_DATA_LEN);

    if (msg_data == NULL) {
        ESP_LOGE(I2C_MSG_TAG, "Failed to allocate memory for message data");
        return;
    }

    msg_data[0] = msg;
    msg_data[1] = dev_idx;
    msg_data[2] = data_len;

    memcpy(msg_data + HEADER_LEN, data, data_len);

    for (size_t i = HEADER_LEN + data_len; i < I2C_DATA_LEN; i++) {
        msg_data[i] = 0xFF;
    }

    i2c_write(msg_data);

    #if LOG_I2C == 0
    ESP_LOGI(I2C_MSG_TAG, "Message sent: %d", msg);
    esp_log_buffer_hex(I2C_MSG_TAG, msg_data, message_len);
    #endif

    free(msg_data);
}