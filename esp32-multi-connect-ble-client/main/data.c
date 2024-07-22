#include "data.h"

void m_node_cb(size_t device_idx, uint8_t *value, uint16_t value_len) {
    ESP_LOGI(DATA_TAG, "Device %d: M node data received", device_idx);
    esp_log_buffer_hex(DATA_TAG, value, value_len);
}

void a_node_cb(size_t device_idx, uint8_t *value, uint16_t value_len) {
    ESP_LOGI(DATA_TAG, "Device %d: A node data received", device_idx);
    esp_log_buffer_hex(DATA_TAG, value, value_len);
}