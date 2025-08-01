#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#define MAIN_TAG "SUB_MAIN"

void uptime_task(void *pvParameters) {
    while (1) {
        uint32_t uptime_ms = esp_timer_get_time() / 1000;
        uint32_t uptime_seconds = uptime_ms / 1000;
        uint32_t hours = uptime_seconds / 3600;
        uint32_t minutes = (uptime_seconds % 3600) / 60;
        uint32_t seconds = uptime_seconds % 60;
        
        ESP_LOGI(MAIN_TAG, "Uptime: %02lu:%02lu:%02lu (%lu seconds)", hours, minutes, seconds, uptime_seconds);
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // Log uptime every 5 seconds
    }
}

void app_main(void) {
    ESP_LOGI(MAIN_TAG, "SUB node starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(MAIN_TAG, "Starting uptime logging...");
    
    // Create task to log uptime every 5 seconds
    xTaskCreate(uptime_task, "uptime_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(MAIN_TAG, "SUB node initialized successfully");
}