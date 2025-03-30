#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "constants.h"
#include "i2c_master.h"
#include "i2c_messages.h"
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
        
        ESP_LOGI(MAIN_TAG, "Sending dummy data to sub node: %d", counter);
        i2c_send_message_data(msg_data, 0, dummy_data, 4);
        
        // Every 5 messages, send a data request
        if (counter % 5 == 0) {
            ESP_LOGI(MAIN_TAG, "Requesting data from sub node");
            i2c_send_message(msg_req_data, 0);
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Send data every 2 seconds
    }
}

void app_main(void) {
    ESP_LOGI(MAIN_TAG, "DOM (I2C Master) node starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(MAIN_TAG, "Initializing I2C Master...");
    int retry_count = 0;
    const int max_retries = 3;
    
    while (retry_count < max_retries) {
        if (i2c_init()) {
            ESP_LOGI(MAIN_TAG, "I2C Master initialized successfully");
            break;
        }
        
        ESP_LOGW(MAIN_TAG, "Failed to initialize I2C Master (attempt %d/%d), retrying...", 
                 retry_count + 1, max_retries);
        retry_count++;
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait a second before retrying
    }
    
    if (retry_count >= max_retries) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize I2C Master after %d attempts", max_retries);
        return;
    }
    
    ESP_LOGI(MAIN_TAG, "Starting communication with SUB node...");
    i2c_send_message(msg_init_start, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    i2c_send_message(msg_init_end, 0);
    
    // Create task to send dummy data periodically
    xTaskCreate(send_dummy_data_task, "send_dummy_data", 4096, NULL, 5, NULL);
    
    ESP_LOGI(MAIN_TAG, "DOM node initialized successfully");
}