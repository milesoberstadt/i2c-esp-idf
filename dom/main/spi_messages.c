#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "spi_master.h"
#include "spi_messages.h"
#include "types.h"

#define TAG "SPI_MSG"

// Send a message to a specific device
bool spi_msg_send(message_t msg_type, uint8_t device_index) {
    bool success = spi_send_message(msg_type, device_index);
    
    if (!success) {
        ESP_LOGW(TAG, "Received error response: 0xFF from device %d", device_index);
    }
    
    return success;
}

// Send a message with data to a specific device
bool spi_msg_send_data(message_t msg_type, uint8_t device_index, const uint8_t *data, size_t data_len) {
    bool success = spi_send_message_data(msg_type, device_index, data, data_len);
    
    if (!success) {
        ESP_LOGW(TAG, "Received error response: 0xFF from device %d", device_index);
    }
    
    ESP_LOGI(TAG, "Sending message type 0x%02X with %d bytes of data to device %d", 
            msg_type, data_len, device_index);
    
    return success;
}

// Request data from a specific device
bool spi_msg_request_data(uint8_t device_index, uint8_t *rx_data, size_t *rx_data_len) {
    ESP_LOGI(TAG, "Requesting data from device %d", device_index);
    
    bool success = spi_request_data(device_index, rx_data, rx_data_len);
    
    if (!success) {
        ESP_LOGW(TAG, "Failed to retrieve data from device %d", device_index);
    } else {
        ESP_LOGI(TAG, "Received %d bytes of data from device %d", *rx_data_len, device_index);
    }
    
    return success;
}

// Set WiFi channel for a specific device
bool spi_msg_set_wifi_channel(uint8_t device_index, uint8_t channel) {
    ESP_LOGI(TAG, "Setting WiFi channel to %d for device %d", channel, device_index);
    
    bool success = spi_set_wifi_channel(channel);
    
    if (!success) {
        ESP_LOGW(TAG, "Failed to set WiFi channel for device %d", device_index);
    } else {
        ESP_LOGI(TAG, "Successfully set WiFi channel for device %d", device_index);
    }
    
    return success;
}