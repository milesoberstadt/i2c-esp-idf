#include "i2c_messages.h"

void i2c_send_message(message_t msg, size_t dev_idx) {
    i2c_send_message_data(msg, dev_idx, NULL, 0);
}

void i2c_send_message_data(message_t msg, size_t dev_idx, uint8_t *data, size_t data_len) {

    // don't compute the message if there is nobody to send it to
    if (!get_is_screen_on()) {
        return;
    }

    size_t message_header_len = 2;
    size_t message_len = data_len + message_header_len;

    uint8_t *msg_data = (uint8_t *)malloc(message_len);

    if (msg_data == NULL) {
        ESP_LOGE(I2C_MSG_TAG, "Failed to allocate memory for message data");
        return;
    }

    msg_data[0] = msg;
    msg_data[1] = dev_idx;

    memcpy(msg_data + message_header_len, data, data_len);

    if (get_is_screen_on()) {
        i2c_master_write_slave(msg_data, message_len, I2C_SCREEN_SLAVE_ADDR);
    }

    // i2c_master_write_slave(msg_data, message_len, I2C_WIFI_SLAVE_ADDR);

    free(msg_data);
}