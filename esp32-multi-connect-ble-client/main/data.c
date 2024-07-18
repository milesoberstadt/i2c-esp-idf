#include "data.h"

void gyro_data_callback(size_t device_idx, uint8_t *value, uint16_t value_len) {

    ESP_LOGI(DATA_TAG, "Device %d: Gyro data received", device_idx);
    esp_log_buffer_hex(DATA_TAG, value, value_len);

}