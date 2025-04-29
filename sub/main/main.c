#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "constants.h"
#include "spi_slave.h"
#include "spi_messages.h"
#include "types.h"
#include "wifi_sniffer.h"

#define MAIN_TAG "SUB_MAIN"

void app_main(void) {
    ESP_LOGI(MAIN_TAG, "SUB (SPI Slave) node starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize WiFi in station mode for sniffing
    ESP_LOGI(MAIN_TAG, "Initializing WiFi sniffer...");
    if (!wifi_sniffer_init()) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize WiFi sniffer");
        // Continue anyway, SPI communication can still work
    } else {
        ESP_LOGI(MAIN_TAG, "WiFi sniffer initialized successfully");
        // Set default channel
        wifi_sniffer_set_channel(DEFAULT_WIFI_CHANNEL);
    }

    // Create a task to handle sniffer events
    xTaskCreate(wifi_sniffer_event_handler_task, "sniffer_events", 4096, NULL, 5, NULL);
    
    ESP_LOGI(MAIN_TAG, "Initializing SPI Slave...");
    
    // Initialize SPI slave
    if (!spi_slave_init()) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize SPI slave");
        return;
    }
    
    ESP_LOGI(MAIN_TAG, "Starting SPI receive tasks...");
    if (!spi_slave_start_tasks()) {
        ESP_LOGE(MAIN_TAG, "Failed to start SPI receive task");
        return;
    }
    
    ESP_LOGI(MAIN_TAG, "SUB node initialized successfully");
    ESP_LOGI(MAIN_TAG, "Waiting for messages from DOM node...");
    
    // Main program loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // Just keep the program running
    }
}