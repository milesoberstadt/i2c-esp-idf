#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "constants.h"
#include "spi_master.h"
#include "spi_messages.h"
#include "types.h"

#define MAIN_TAG "DOM_MAIN"

void send_dummy_data_task(void *pvParameters) {
    uint8_t counter = 0;
    
    while (1) {
        uint8_t dummy_data[4];
        dummy_data[0] = counter++;
        dummy_data[1] = 0xAA;
        dummy_data[2] = 0xBB;
        dummy_data[3] = 0xCC;
        
        ESP_LOGI(MAIN_TAG, "Sending dummy data to sub node: counter=%d", counter);
        spi_send_message_data(msg_data, 0, dummy_data, 4);
        
        // Every 5 messages, send a data request
        if (counter % 5 == 0) {
            ESP_LOGI(MAIN_TAG, "Requesting data from sub node");
            
            uint8_t rx_data[32];
            size_t rx_data_len = sizeof(rx_data);
            
            if (spi_request_data(0, rx_data, &rx_data_len)) {
                ESP_LOGI(MAIN_TAG, "Received %d bytes of data from sub node", rx_data_len);
                
                // Log first few bytes of data
                if (rx_data_len > 0) {
                    ESP_LOGI(MAIN_TAG, "Data: [%02X %02X %02X %02X ...]",
                             rx_data_len > 0 ? rx_data[0] : 0xFF,
                             rx_data_len > 1 ? rx_data[1] : 0xFF,
                             rx_data_len > 2 ? rx_data[2] : 0xFF,
                             rx_data_len > 3 ? rx_data[3] : 0xFF);
                }
            } else {
                ESP_LOGW(MAIN_TAG, "Failed to receive data from sub node");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Send data every 2 seconds
    }
}

void app_main(void) {
    ESP_LOGI(MAIN_TAG, "DOM (SPI Master) node starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(MAIN_TAG, "Initializing SPI Master...");
    int retry_count = 0;
    const int max_retries = 3;
    
    while (retry_count < max_retries) {
        if (spi_master_init()) {
            ESP_LOGI(MAIN_TAG, "SPI Master initialized successfully");
            
            // Check if the slave is responsive
            if (spi_check_slave()) {
                ESP_LOGI(MAIN_TAG, "==================================");
                ESP_LOGI(MAIN_TAG, "FOUND SUB NODE!");
                ESP_LOGI(MAIN_TAG, "==================================");
                break;
            } else {
                ESP_LOGW(MAIN_TAG, "No sub node found or not responsive");
                retry_count++;
                vTaskDelay(pdMS_TO_TICKS(1000)); // Wait a second before retrying
            }
        } else {
            ESP_LOGW(MAIN_TAG, "Failed to initialize SPI Master (attempt %d/%d), retrying...", 
                    retry_count + 1, max_retries);
            retry_count++;
            vTaskDelay(pdMS_TO_TICKS(1000)); // Wait a second before retrying
        }
    }
    
    if (retry_count >= max_retries) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize SPI Master after %d attempts", max_retries);
        return;
    }
    
    ESP_LOGI(MAIN_TAG, "Starting communication with SUB node...");
    
    // Send initialization messages to the sub node
    spi_send_message(msg_init_start, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Set WiFi channel for the sub node
    ESP_LOGI(MAIN_TAG, "Setting WiFi channel for sub node...");
    
    uint8_t wifi_channel = 6; // Channel 6 is usually less crowded
    if (spi_set_wifi_channel(wifi_channel)) {
        ESP_LOGI(MAIN_TAG, "WiFi channel set to %d", wifi_channel);
    } else {
        ESP_LOGW(MAIN_TAG, "Failed to set WiFi channel");
    }
    
    spi_send_message(msg_init_end, 0);
    
    // Create task to send dummy data periodically
    xTaskCreate(send_dummy_data_task, "send_dummy_data", 4096, NULL, 5, NULL);
    
    ESP_LOGI(MAIN_TAG, "DOM node initialized successfully");
}