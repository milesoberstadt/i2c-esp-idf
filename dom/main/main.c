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
        
        int node_count = i2c_get_connected_node_count();
        
        if (node_count > 0) {
            ESP_LOGI(MAIN_TAG, "Sending dummy data to all %d sub nodes: counter=%d", node_count, counter);
            i2c_broadcast_message_data(msg_data, 0, dummy_data, 4);
            
            // Every 5 messages, send a data request to each node
            if (counter % 5 == 0) {
                ESP_LOGI(MAIN_TAG, "Requesting data from all sub nodes");
                i2c_broadcast_message(msg_req_data, 0);
            }
        } else {
            ESP_LOGW(MAIN_TAG, "No sub nodes connected, skipping data transmission");
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
            
            // Get the number of connected sub nodes
            int node_count = i2c_get_connected_node_count();
            
            if (node_count > 0) {
                ESP_LOGI(MAIN_TAG, "==================================");
                ESP_LOGI(MAIN_TAG, "FOUND %d SUB NODE(S)!", node_count);
                
                // Display information about each connected node
                for (int i = 0; i < MAX_SUB_NODES; i++) {
                    if (i2c_is_node_connected(i)) {
                        const sub_node_t* node = i2c_get_node_info(i);
                        ESP_LOGI(MAIN_TAG, "Node %d: Address=0x%02X, Identifier=0x%02X", 
                                i, node->address, node->identifier);
                    }
                }
                ESP_LOGI(MAIN_TAG, "==================================");
                break;
            } else {
                ESP_LOGW(MAIN_TAG, "No sub nodes found");
                retry_count++;
                vTaskDelay(pdMS_TO_TICKS(1000)); // Wait a second before retrying
            }
        } else {
            ESP_LOGW(MAIN_TAG, "Failed to initialize I2C Master (attempt %d/%d), retrying...", 
                    retry_count + 1, max_retries);
            retry_count++;
            vTaskDelay(pdMS_TO_TICKS(1000)); // Wait a second before retrying
        }
    }
    
    if (retry_count >= max_retries) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize I2C Master after %d attempts", max_retries);
        return;
    }
    
    ESP_LOGI(MAIN_TAG, "Starting communication with all SUB nodes...");
    
    // Send initialization messages to all connected sub nodes
    i2c_broadcast_message(msg_init_start, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    i2c_broadcast_message(msg_init_end, 0);
    
    // Create task to send dummy data periodically
    xTaskCreate(send_dummy_data_task, "send_dummy_data", 4096, NULL, 5, NULL);
    
    ESP_LOGI(MAIN_TAG, "DOM node initialized successfully");
}