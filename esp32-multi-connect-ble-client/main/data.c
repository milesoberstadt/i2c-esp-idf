#include "data.h"

void m_node_cb(size_t device_idx, size_t char_idx, uint8_t *value, uint16_t value_len) {

    if (char_idx == 0) {
        ESP_LOGI(DATA_TAG, "Device %d: M node GYRO received", device_idx);
    }

    if (char_idx == 1) {
        ESP_LOGI(DATA_TAG, "Device %d: M node ACCELL received", device_idx);
    }

    esp_log_buffer_hex(DATA_TAG, value, value_len);
}

void a_node_cb(size_t device_idx, size_t char_idx, uint8_t *value, uint16_t value_len) {
    if (char_idx == 0) {
        ESP_LOGI(DATA_TAG, "Device %d: A node wind speed received", device_idx);
    }

    if (char_idx == 1) {
        ESP_LOGI(DATA_TAG, "Device %d: A node wind direction received", device_idx);
    }

    esp_log_buffer_hex(DATA_TAG, value, value_len);
}