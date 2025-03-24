#include <stdio.h>
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_master.h"
#include "sub_manager.h"
#include "ap_database.h"
#include "types.h"

static const char *TAG = "dom-main";

// Mock timestamp (no GPS yet)
static uint32_t current_timestamp = 0;

// Task handles
static TaskHandle_t sync_task_handle = NULL;
static TaskHandle_t console_task_handle = NULL;

// AP synchronization task
static void sync_ap_data_task(void *pvParameters) {
    const TickType_t interval = pdMS_TO_TICKS(30000); // 30 seconds
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        ESP_LOGI(TAG, "Starting AP data synchronization round");
        
        // Increment mock timestamp (would come from GPS)
        current_timestamp += 30;
        
        // Set timestamp on all SUBs
        sub_manager_set_timestamp(current_timestamp);
        
        // Synchronize AP data from all SUBs
        sub_manager_sync_ap_data();
        
        // Log status
        ESP_LOGI(TAG, "AP data synchronization complete");
        ESP_LOGI(TAG, "SUBs: %u, Total APs: %lu", 
                 sub_manager_get_count(), (unsigned long)sub_manager_get_total_ap_count());
        
        // Wait for the next interval
        vTaskDelayUntil(&last_wake_time, interval);
    }
}

// Console task to display status and handle commands
static void console_task(void *pvParameters) {
    const TickType_t interval = pdMS_TO_TICKS(5000); // 5 seconds
    
    while (1) {
        // Print status header
        printf("\n==== WiFi Scanner DOM Status ====\n");
        printf("Time: %lu seconds\n", (unsigned long)current_timestamp);
        printf("SUBs discovered: %u\n", sub_manager_get_count());
        printf("Total APs found: %lu\n", (unsigned long)sub_manager_get_total_ap_count());
        
        // Print SUB status
        printf("\nSUB Status:\n");
        printf("ID | Addr | Channel | Status | APs Found | Last Seen\n");
        printf("---------------------------------------------------\n");
        
        for (uint8_t i = 0; i < sub_manager_get_count(); i++) {
            sub_info_t *sub = sub_manager_get_sub(i);
            if (sub) {
                const char *status_str;
                switch (sub->status) {
                    case SUB_STATUS_UNINITIALIZED:
                        status_str = "UNINIT";
                        break;
                    case SUB_STATUS_INITIALIZED:
                        status_str = "READY";
                        break;
                    case SUB_STATUS_SCANNING:
                        status_str = "SCANNING";
                        break;
                    case SUB_STATUS_PAUSED:
                        status_str = "PAUSED";
                        break;
                    case SUB_STATUS_ERROR:
                        status_str = "ERROR";
                        break;
                    case SUB_STATUS_DISCONNECTED:
                        status_str = "DISCONNECTED";
                        break;
                    default:
                        status_str = "UNKNOWN";
                        break;
                }
                
                printf("%2u | 0x%02X | %7u | %7s | %8u | %9lu\n",
                       sub->id, sub->i2c_addr, sub->wifi_channel,
                       status_str, sub->ap_count, (unsigned long)sub->last_seen);
            }
        }
        
        // Wait for the next update
        vTaskDelay(interval);
    }
}

// Initialize and start tasks
static void start_tasks(void) {
    // Create AP synchronization task
    xTaskCreate(sync_ap_data_task, "sync_ap", 4096, NULL, 5, &sync_task_handle);
    
    // Create console task
    xTaskCreate(console_task, "console", 4096, NULL, 1, &console_task_handle);
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting WiFi Scanner DOM node");
    
    // Initialize NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize the AP database
    ap_database_init();
    
    // Initialize SUB manager
    ESP_ERROR_CHECK(sub_manager_init());
    
    // Initial timestamp (mock)
    current_timestamp = 1583020800; // March 1, 2020 00:00:00 GMT
    
    // Wait for SUBs to start up and stabilize
    ESP_LOGI(TAG, "Waiting 5 seconds for SUB nodes to initialize...");
    vTaskDelay(pdMS_TO_TICKS(5000));  // 5 second delay
    
    // Discover SUBs
    ESP_LOGI(TAG, "Discovering SUBs...");
    ESP_ERROR_CHECK(sub_manager_discover());
    ESP_LOGI(TAG, "Found %u SUBs", sub_manager_get_count());
    
    // If no SUBs found, try again
    if (sub_manager_get_count() == 0) {
        ESP_LOGW(TAG, "No SUBs found, waiting and trying again...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        ESP_ERROR_CHECK(sub_manager_discover());
        ESP_LOGI(TAG, "Found %u SUBs", sub_manager_get_count());
    }
    
    // Set timestamp on all SUBs
    sub_manager_set_timestamp(current_timestamp);
    
    // Start scanning on all SUBs
    ESP_LOGI(TAG, "Starting scanning on all SUBs");
    ESP_ERROR_CHECK(sub_manager_start_scanning());
    
    // Start tasks
    start_tasks();
    
    ESP_LOGI(TAG, "DOM node initialization complete");
}