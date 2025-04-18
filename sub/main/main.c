#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "constants.h"
#include "i2c_slave.h"
#include "i2c_messages.h"
#include "types.h"
#include "wifi_sniffer.h"

#define MAIN_TAG "SUB_MAIN"

void app_main(void) {
    ESP_LOGI(MAIN_TAG, "SUB (I2C Slave) node starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(MAIN_TAG, "Initializing I2C Slave...");
    
    // Don't create a separate task for I2C initialization, just call it directly
    i2c_slave_init();
    
    ESP_LOGI(MAIN_TAG, "Starting I2C receive tasks...");
    i2c_start_receive();
    
    ESP_LOGI(MAIN_TAG, "SUB node initialized successfully");
    ESP_LOGI(MAIN_TAG, "Waiting for messages from DOM node...");
    
    // Create a task to handle sniffer events
    xTaskCreate(wifi_sniffer_event_handler_task, "sniffer_events", 4096, NULL, 5, NULL);
    
    // Main program loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // Just keep the program running
    }
}