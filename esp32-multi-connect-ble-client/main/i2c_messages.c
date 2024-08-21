#include "i2c_messages.h"

#define HEADER_LEN 3

void i2c_send_message(message_t msg, size_t dev_idx) {
    i2c_send_message_data(msg, dev_idx, NULL, 0);
}

void i2c_send_message_data(message_t msg, size_t dev_idx, uint8_t *data, size_t data_len) {

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

    memcpy(msg_data + HEADER_LEN, data, data_len);

    for (size_t i = HEADER_LEN + data_len; i < I2C_DATA_LEN; i++) {
        msg_data[i] = 0xFF;
    }

    i2c_write(msg_data);

    #if LOG_I2C == 1
    ESP_LOGI(I2C_MSG_TAG, "Message sent: %d", msg);
    esp_log_buffer_hex(I2C_MSG_TAG, msg_data, I2C_DATA_LEN);
    #endif

    free(msg_data);
}

#define HEADER_SIZE 3

void process_message(uint8_t* data, size_t length) {

    esp_log_buffer_hex(I2C_MSG_TAG, data, length);

    uint8_t msg_type = data[0];
    uint8_t dev_idx = data[1];
    uint8_t msg_len = data[3];

    ESP_LOGI(I2C_MSG_TAG, "Message type: %d\n", msg_type);

    // Handle devices message
    switch (msg_type) {
        case msg_req_dev:

            size_t dev_idx = data[4];
            device_t dev;
            get_cache_device(&dev, dev_idx);

            uint8_t res[3];
            res[0]=dev.type;
            res[1]=dev.state;
            res[2]=dev.battery_level;

            i2c_send_message_data(msg_res_dev, dev_idx, res, 3);

        default:
            break;
    }
}
