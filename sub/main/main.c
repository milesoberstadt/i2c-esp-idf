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

// Forward declarations
static void sniffer_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data);
void wifi_sniffer_event_handler_task(void *pvParameters);

/**
 * @brief Task to handle WiFi sniffer events
 * 
 * This task registers for all sniffer events and processes them
 */
void wifi_sniffer_event_handler_task(void *pvParameters) {
    ESP_LOGI(MAIN_TAG, "Starting WiFi sniffer event handler task");
    
    // Create event loop for handling WiFi events
    esp_event_loop_handle_t event_loop_handle;
    esp_event_loop_args_t event_loop_args = {
        .queue_size = 10,
        .task_name = "sniffer_event_loop",
        .task_priority = 5,
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY
    };
    
    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &event_loop_handle));
    
    // Register for all sniffer events
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        event_loop_handle,
        SNIFFER_EVENTS,
        ESP_EVENT_ANY_ID,
        sniffer_event_handler,
        NULL));
    
    ESP_LOGI(MAIN_TAG, "Registered for sniffer events");
    
    // Keep task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Event handler for WiFi sniffer events
 */
static void sniffer_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data) {
    if (event_base != SNIFFER_EVENTS) {
        return;
    }
    
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *)event_data;
    
    switch (event_id) {
        case SNIFFER_EVENT_BEACON:
            ESP_LOGI(MAIN_TAG, "Beacon frame detected: RSSI=%" PRId8 ", Channel=%" PRId8, 
                     frame->rx_ctrl.rssi, 
                     frame->rx_ctrl.channel);
            
            // For now, we're just logging beacon detections
            // In the future, this is where we could send beacon data to the DOM node via I2C
            break;
            
        case SNIFFER_EVENT_CAPTURED_MGMT:
            ESP_LOGD(MAIN_TAG, "Management frame detected");
            break;
            
        case SNIFFER_EVENT_CAPTURED_DATA:
            ESP_LOGD(MAIN_TAG, "Data frame detected");
            break;
            
        case SNIFFER_EVENT_CAPTURED_CTRL:
            ESP_LOGD(MAIN_TAG, "Control frame detected");
            break;
            
        default:
            ESP_LOGW(MAIN_TAG, "Unknown sniffer event: %" PRId32, event_id);
            break;
    }
}

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
    
    // Initialize WiFi in station mode for sniffing
    wifi_sniffer_init();
    
    ESP_LOGI(MAIN_TAG, "SUB node initialized successfully");
    ESP_LOGI(MAIN_TAG, "Waiting for messages from DOM node...");
    
    // Create a task to handle sniffer events
    xTaskCreate(wifi_sniffer_event_handler_task, "sniffer_events", 4096, NULL, 5, NULL);
    
    // Main program loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // Just keep the program running
    }
}