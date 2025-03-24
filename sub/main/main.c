#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_slave.h"
#include "wifi_scanner.h"
#include "ap_list.h"
#include "types.h"

static const char *TAG = "sub-main";

void app_main(void) {
    ESP_LOGI(TAG, "Starting WiFi Scanner SUB node");
    
    // Initialize NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize AP list
    ap_list_init();
    
    // Initialize WiFi in promiscuous mode
    ESP_ERROR_CHECK(wifi_scanner_init());
    
    // Initialize I2C slave with random address
    ESP_ERROR_CHECK(i2c_slave_init());
    
    // Start I2C slave
    ESP_ERROR_CHECK(i2c_slave_start());
    
    // Log our configuration
    sub_config_t *config = i2c_slave_get_config();
    ESP_LOGI(TAG, "SUB node initialized with ID: %s, I2C address: 0x%02X", 
             config->id, config->i2c_addr);
    
    // Give our I2C system time to fully initialize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "SUB NODE READY - Listening for DOM discovery on I2C address 0x%02X", config->i2c_addr);
    ESP_LOGI(TAG, "Waiting for DOM node to discover us...");
    
    // Main loop - nothing to do here, everything is event-driven
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Log status periodically
        config = i2c_slave_get_config();
        ESP_LOGI(TAG, "Status: %d, WiFi channel: %d, I2C address: 0x%02X, Verified: %s",
                 config->status, config->wifi_channel, config->i2c_addr,
                 i2c_slave_is_verified() ? "Yes" : "No");
        
        // If scanning, log AP count
        if (config->status == SUB_STATUS_SCANNING) {
            ESP_LOGI(TAG, "APs found: %u", ap_list_get_count());
        }
    }
}