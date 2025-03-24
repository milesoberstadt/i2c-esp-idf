#include "sub_manager.h"
#include "i2c_master.h"
#include "ap_database.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "sub_manager";

// Array of SUB information
static sub_info_t subs[MAX_SUB_NODES];
static uint8_t sub_count = 0;

// Total AP count
static uint32_t total_ap_count = 0;

// Manager state
static bool is_busy = false;
static SemaphoreHandle_t manager_mutex = NULL;

esp_err_t sub_manager_init(void) {
    ESP_LOGI(TAG, "Initializing SUB manager");
    
    // Clear SUB array
    memset(subs, 0, sizeof(subs));
    sub_count = 0;
    total_ap_count = 0;
    
    // Create mutex for thread safety
    manager_mutex = xSemaphoreCreateMutex();
    if (manager_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }
    
    // Initialize I2C master
    ESP_ERROR_CHECK(i2c_master_init());
    
    ESP_LOGI(TAG, "SUB manager initialized");
    return ESP_OK;
}

static uint8_t get_next_i2c_addr(void) {
    static uint8_t next_addr = I2C_ADDR_RESERVED_MIN;
    
    // Get next available address
    uint8_t addr = next_addr++;
    
    // If we've reached the end of the range, reset
    if (next_addr > I2C_ADDR_RESERVED_MAX) {
        next_addr = I2C_ADDR_RESERVED_MIN;
    }
    
    return addr;
}

static uint8_t get_next_wifi_channel(void) {
    // Assign WiFi channels from 1-11
    // Ensure we don't assign the same channel to adjacent SUBs
    static uint8_t channels[] = {1, 6, 11, 2, 7, 3, 8, 4, 9, 5, 10};
    static uint8_t channel_idx = 0;
    
    // Get next channel
    uint8_t channel = channels[channel_idx++];
    
    // Wrap around to the beginning
    if (channel_idx >= sizeof(channels)) {
        channel_idx = 0;
    }
    
    return channel;
}

esp_err_t sub_manager_discover(void) {
    ESP_LOGI(TAG, "Starting SUB discovery");
    
    if (xSemaphoreTake(manager_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_FAIL;
    }
    
    is_busy = true;
    
    // Array to store found I2C addresses
    uint8_t found_addrs[128];
    uint8_t num_found = 0;
    bool collision_detected = false;
    
    // Scan for devices in the random address range
    ESP_LOGI(TAG, "Scanning I2C bus for SUB devices in address range 0x%02X-0x%02X", 
             I2C_ADDR_RANDOM_MIN, I2C_ADDR_RANDOM_MAX);
    ESP_ERROR_CHECK(i2c_master_scan(I2C_ADDR_RANDOM_MIN, I2C_ADDR_RANDOM_MAX, found_addrs, &num_found));
    
    ESP_LOGI(TAG, "Found %u potential SUB devices in random address range", num_found);
    
    // Process each found device
    for (uint8_t i = 0; i < num_found && sub_count < MAX_SUB_NODES; i++) {
        uint8_t addr = found_addrs[i];
        i2c_message_t response;
        esp_err_t ret;
        
        ESP_LOGI(TAG, "Probing device at address 0x%02X with HELLO message", addr);
        
        // Send hello message
        ret = i2c_master_send_hello(addr);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send HELLO to device at 0x%02X: %s", addr, esp_err_to_name(ret));
            continue;
        }
        
        ESP_LOGI(TAG, "HELLO message sent successfully to 0x%02X, waiting for response...", addr);
        
        // Longer delay to allow SUB time to process and respond (increase from 10ms to 50ms)
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // Read response
        ESP_LOGI(TAG, "Attempting to read response from device at 0x%02X", addr);
        ret = i2c_master_read_msg(addr, &response);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to read response from 0x%02X: %s", addr, esp_err_to_name(ret));
            continue;
        }
        
        ESP_LOGI(TAG, "Successfully read response from 0x%02X, msg_type=0x%02X, data_len=%u", 
                addr, response.header.msg_type, response.header.data_len);
        
        // Check if it's a valid SUB response
        if (response.header.msg_type != MSG_HELLO || response.header.data_len < 2) {
            ESP_LOGW(TAG, "Invalid response from 0x%02X", addr);
            continue;
        }
        
        // Extract ID string from response
        char id_str[3] = {0};
        memcpy(id_str, response.data, 2);
        id_str[2] = '\0';
        
        ESP_LOGI(TAG, "Device at 0x%02X responded with ID: %s", addr, id_str);
        
        // Verify ID string
        ret = i2c_master_send_verify(addr, id_str);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send verify to 0x%02X", addr);
            continue;
        }
        
        // Small delay for SUB to respond
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Read verification response
        ret = i2c_master_read_msg(addr, &response);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to read verification response from 0x%02X", addr);
            continue;
        }
        
        // Check verification response
        if (response.header.msg_type != MSG_VERIFY || 
            response.header.data_len < 2 ||
            memcmp(response.data, id_str, 2) != 0) {
            
            ESP_LOGW(TAG, "Verification failed for 0x%02X", addr);
            
            // Check if this indicates a collision (multiple SUBs responding)
            if (response.header.msg_type == MSG_ERROR || response.header.data_len != 2) {
                collision_detected = true;
            }
            
            continue;
        }
        
        ESP_LOGI(TAG, "Device at 0x%02X verified with ID: %s", addr, id_str);
        
        // Assign new address, WiFi channel, and SUB ID
        uint8_t new_addr = get_next_i2c_addr();
        uint8_t wifi_channel = get_next_wifi_channel();
        uint8_t sub_id = sub_count;
        
        ret = i2c_master_send_assign(addr, new_addr, wifi_channel, sub_id);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to assign new address to 0x%02X", addr);
            continue;
        }
        
        // Small delay for SUB to respond
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Read assignment response
        ret = i2c_master_read_msg(addr, &response);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to read assignment response from 0x%02X", addr);
            continue;
        }
        
        // Check assignment response
        if (response.header.msg_type != MSG_ASSIGN) {
            ESP_LOGW(TAG, "Assignment failed for 0x%02X", addr);
            continue;
        }
        
        ESP_LOGI(TAG, "Successfully assigned SUB %u: 0x%02X -> 0x%02X, WiFi channel %u",
                 sub_id, addr, new_addr, wifi_channel);
        
        // Wait for SUB to change address
        vTaskDelay(pdMS_TO_TICKS(200));
        
        // Add SUB to our list
        subs[sub_count].id = sub_id;
        subs[sub_count].i2c_addr = new_addr;
        subs[sub_count].wifi_channel = wifi_channel;
        memcpy(subs[sub_count].id_str, id_str, 3);
        subs[sub_count].status = SUB_STATUS_INITIALIZED;
        subs[sub_count].ap_count = 0;
        subs[sub_count].last_seen = xTaskGetTickCount();
        
        sub_count++;
    }
    
    ESP_LOGI(TAG, "SUB discovery completed, found %u SUBs", sub_count);
    
    // If we detected a collision, reset all SUBs and try again
    if (collision_detected && sub_count == 0) {
        ESP_LOGW(TAG, "Collision detected, resetting all SUBs");
        
        // Broadcast reset command to all devices in random range
        for (uint8_t addr = I2C_ADDR_RANDOM_MIN; addr <= I2C_ADDR_RANDOM_MAX; addr++) {
            i2c_master_send_reset(addr);
        }
        
        // Wait for SUBs to reset
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Retry discovery recursively (just once)
        is_busy = false;
        xSemaphoreGive(manager_mutex);
        return sub_manager_discover();
    }
    
    is_busy = false;
    xSemaphoreGive(manager_mutex);
    
    return ESP_OK;
}

esp_err_t sub_manager_set_timestamp(uint32_t timestamp) {
    ESP_LOGI(TAG, "Setting timestamp %lu on all SUBs", (unsigned long)timestamp);
    
    if (xSemaphoreTake(manager_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_FAIL;
    }
    
    is_busy = true;
    esp_err_t result = ESP_OK;
    
    for (uint8_t i = 0; i < sub_count; i++) {
        if (subs[i].status == SUB_STATUS_INITIALIZED || 
            subs[i].status == SUB_STATUS_SCANNING) {
            
            esp_err_t ret = i2c_master_set_time(subs[i].i2c_addr, subs[i].id, timestamp);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to set timestamp on SUB %u", subs[i].id);
                subs[i].status = SUB_STATUS_ERROR;
                result = ESP_FAIL;
            }
        }
    }
    
    is_busy = false;
    xSemaphoreGive(manager_mutex);
    
    return result;
}

esp_err_t sub_manager_start_scanning(void) {
    ESP_LOGI(TAG, "Starting scanning on all SUBs");
    
    if (xSemaphoreTake(manager_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_FAIL;
    }
    
    is_busy = true;
    esp_err_t result = ESP_OK;
    
    for (uint8_t i = 0; i < sub_count; i++) {
        if (subs[i].status == SUB_STATUS_INITIALIZED) {
            esp_err_t ret = i2c_master_start_scan(subs[i].i2c_addr, subs[i].id);
            if (ret == ESP_OK) {
                subs[i].status = SUB_STATUS_SCANNING;
            } else {
                ESP_LOGW(TAG, "Failed to start scanning on SUB %u", subs[i].id);
                subs[i].status = SUB_STATUS_ERROR;
                result = ESP_FAIL;
            }
        }
    }
    
    is_busy = false;
    xSemaphoreGive(manager_mutex);
    
    return result;
}

esp_err_t sub_manager_stop_scanning(void) {
    ESP_LOGI(TAG, "Stopping scanning on all SUBs");
    
    if (xSemaphoreTake(manager_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_FAIL;
    }
    
    is_busy = true;
    esp_err_t result = ESP_OK;
    
    for (uint8_t i = 0; i < sub_count; i++) {
        if (subs[i].status == SUB_STATUS_SCANNING) {
            esp_err_t ret = i2c_master_stop_scan(subs[i].i2c_addr, subs[i].id);
            if (ret == ESP_OK) {
                subs[i].status = SUB_STATUS_INITIALIZED;
            } else {
                ESP_LOGW(TAG, "Failed to stop scanning on SUB %u", subs[i].id);
                subs[i].status = SUB_STATUS_ERROR;
                result = ESP_FAIL;
            }
        }
    }
    
    is_busy = false;
    xSemaphoreGive(manager_mutex);
    
    return result;
}

esp_err_t sub_manager_sync_ap_data(void) {
    ESP_LOGI(TAG, "Synchronizing AP data from all SUBs");
    
    if (xSemaphoreTake(manager_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_FAIL;
    }
    
    is_busy = true;
    esp_err_t result = ESP_OK;
    
    // Reset total AP count
    total_ap_count = 0;
    
    // Process each SUB
    for (uint8_t i = 0; i < sub_count; i++) {
        if (subs[i].status != SUB_STATUS_INITIALIZED && 
            subs[i].status != SUB_STATUS_SCANNING) {
            continue;
        }
        
        // Get AP count
        uint16_t count = 0;
        esp_err_t ret = i2c_master_req_ap_count(subs[i].i2c_addr, subs[i].id, &count);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to get AP count from SUB %u", subs[i].id);
            subs[i].status = SUB_STATUS_ERROR;
            result = ESP_FAIL;
            continue;
        }
        
        // Update AP count
        subs[i].ap_count = count;
        total_ap_count += count;
        
        ESP_LOGI(TAG, "SUB %u reported %u APs", subs[i].id, count);
        
        // Retrieve each AP record
        ap_record_t record;
        bool has_more = true;
        uint16_t records_synced = 0;
        
        while (has_more) {
            ret = i2c_master_req_ap_data(subs[i].i2c_addr, subs[i].id, &record, &has_more);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to get AP data from SUB %u", subs[i].id);
                break;
            }
            
            if (!has_more) {
                break;
            }
            
            // Add record to database
            if (ap_database_add(&record)) {
                // Confirm receipt
                ret = i2c_master_confirm_ap(subs[i].i2c_addr, subs[i].id, record.bssid);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to confirm AP data on SUB %u", subs[i].id);
                    break;
                }
                
                records_synced++;
            }
        }
        
        ESP_LOGI(TAG, "Synced %u of %u APs from SUB %u", 
                 records_synced, subs[i].ap_count, subs[i].id);
        
        // Update last seen time
        subs[i].last_seen = xTaskGetTickCount();
    }
    
    is_busy = false;
    xSemaphoreGive(manager_mutex);
    
    ESP_LOGI(TAG, "AP data synchronization complete, total APs: %lu", (unsigned long)total_ap_count);
    return result;
}

uint8_t sub_manager_get_count(void) {
    return sub_count;
}

sub_info_t *sub_manager_get_sub(uint8_t index) {
    if (index >= sub_count) {
        return NULL;
    }
    
    return &subs[index];
}

uint32_t sub_manager_get_total_ap_count(void) {
    return total_ap_count;
}

esp_err_t sub_manager_reset_all(void) {
    ESP_LOGI(TAG, "Resetting all SUBs");
    
    if (xSemaphoreTake(manager_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_FAIL;
    }
    
    is_busy = true;
    
    // Reset all SUBs in both address ranges
    for (uint8_t addr = I2C_ADDR_RANDOM_MIN; addr <= I2C_ADDR_RANDOM_MAX; addr++) {
        i2c_master_send_reset(addr);
    }
    
    for (uint8_t addr = I2C_ADDR_RESERVED_MIN; addr <= I2C_ADDR_RESERVED_MAX; addr++) {
        i2c_master_send_reset(addr);
    }
    
    // Reset state
    memset(subs, 0, sizeof(subs));
    sub_count = 0;
    total_ap_count = 0;
    
    is_busy = false;
    xSemaphoreGive(manager_mutex);
    
    ESP_LOGI(TAG, "All SUBs reset");
    return ESP_OK;
}

bool sub_manager_is_busy(void) {
    return is_busy;
}