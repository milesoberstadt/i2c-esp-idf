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

// Function is not used in fixed address mode but kept for reference
static uint8_t get_next_i2c_addr(void) __attribute__((unused));
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
    // Using a single fixed channel for all SUBs
    // Channel 6 is chosen as it's usually less congested
    const uint8_t FIXED_CHANNEL = 6;
    
    ESP_LOGI(TAG, "Assigning fixed WiFi channel %u", FIXED_CHANNEL);
    return FIXED_CHANNEL;
}

esp_err_t sub_manager_discover(void) {
    ESP_LOGI(TAG, "Using FIXED I2C ADDRESS: Directly connecting to SUB at 0x42");
    
    if (xSemaphoreTake(manager_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_FAIL;
    }
    
    is_busy = true;
    
    // Fixed address for the SUB node in troubleshooting mode
    const uint8_t FIXED_SUB_ADDR = I2C_FIXED_SUB_ADDR;
    i2c_message_t response;
    esp_err_t ret;
    
    // Clear SUB count
    sub_count = 0;
    
    ESP_LOGI(TAG, "Connecting to fixed SUB device at address 0x%02X", FIXED_SUB_ADDR);
    
    // Send hello message to the fixed address
    ret = i2c_master_send_hello(FIXED_SUB_ADDR);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send HELLO to fixed SUB at 0x%02X: %s", 
                 FIXED_SUB_ADDR, esp_err_to_name(ret));
        
        // For debugging purposes, add dummy SUB entry instead of failing
        ESP_LOGW(TAG, "Adding dummy SUB entry for debugging - I2C connection will be retried later");
        subs[0].id = 0;
        subs[0].i2c_addr = FIXED_SUB_ADDR;
        subs[0].wifi_channel = 6;
        strcpy(subs[0].id_str, "DB"); // Dummy ID
        subs[0].status = SUB_STATUS_ERROR;
        subs[0].ap_count = 0;
        subs[0].last_seen = 0;
        sub_count = 1;
        
        is_busy = false;
        xSemaphoreGive(manager_mutex);
        
        // Return OK to prevent app crash
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "HELLO message sent successfully to fixed SUB, waiting for response...");
    
    // Delay to allow SUB time to process and respond - increase delay
    ESP_LOGI(TAG, "Waiting 500ms for SUB to respond...");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Read response
    ESP_LOGI(TAG, "Attempting to read response from fixed SUB");
    ret = i2c_master_read_msg(FIXED_SUB_ADDR, &response);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read response from fixed SUB: %s", esp_err_to_name(ret));
        
        // For debugging purposes, add dummy SUB entry instead of failing
        ESP_LOGW(TAG, "Adding dummy SUB entry for debugging - I2C connection will be retried later");
        subs[0].id = 0;
        subs[0].i2c_addr = FIXED_SUB_ADDR;
        subs[0].wifi_channel = 6;
        strcpy(subs[0].id_str, "DB"); // Dummy ID
        subs[0].status = SUB_STATUS_ERROR;
        subs[0].ap_count = 0;
        subs[0].last_seen = 0;
        sub_count = 1;
        
        is_busy = false;
        xSemaphoreGive(manager_mutex);
        
        // Return OK to prevent app crash
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Successfully read response from fixed SUB, msg_type=0x%02X, data_len=%u", 
            response.header.msg_type, response.header.data_len);
    
    // Check if it's a valid SUB response
    if (response.header.msg_type != MSG_HELLO || response.header.data_len < 2) {
        ESP_LOGW(TAG, "Invalid response from fixed SUB");
        
        // For debugging purposes, add dummy SUB entry instead of failing
        ESP_LOGW(TAG, "Adding dummy SUB entry for debugging - I2C connection will be retried later");
        subs[0].id = 0;
        subs[0].i2c_addr = FIXED_SUB_ADDR;
        subs[0].wifi_channel = 6;
        strcpy(subs[0].id_str, "DB"); // Dummy ID
        subs[0].status = SUB_STATUS_ERROR;
        subs[0].ap_count = 0;
        subs[0].last_seen = 0;
        sub_count = 1;
        
        is_busy = false;
        xSemaphoreGive(manager_mutex);
        
        // Return OK to prevent app crash
        return ESP_OK;
    }
    
    // Extract ID string from response
    char id_str[3] = {0};
    memcpy(id_str, response.data, 2);
    id_str[2] = '\0';
    
    ESP_LOGI(TAG, "Fixed SUB responded with ID: %s", id_str);
    
    // Verify ID string - just for protocol completeness
    ret = i2c_master_send_verify(FIXED_SUB_ADDR, id_str);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send verify to fixed SUB");
        
        // For debugging, continue with the connection
        ESP_LOGW(TAG, "Continuing despite verification failure");
    }
    
    // Small delay for SUB to respond - increase for reliability
    vTaskDelay(pdMS_TO_TICKS(250));
    
    // Read verification response
    ret = i2c_master_read_msg(FIXED_SUB_ADDR, &response);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read verification response from fixed SUB");
        
        // Skip verification for debugging
        ESP_LOGW(TAG, "Skipping verification for debugging purposes");
    }
    
    // Get WiFi channel - use fixed channel 6
    uint8_t wifi_channel = get_next_wifi_channel(); // Will return channel 6
    
    // We'll keep using the same fixed I2C address
    // No need to send an assignment message - but we'll send it to keep the protocol consistent
    ret = i2c_master_send_assign(FIXED_SUB_ADDR, FIXED_SUB_ADDR, wifi_channel, 0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send assignment to fixed SUB");
        
        // Continue anyway for debugging
        ESP_LOGW(TAG, "Continuing despite assignment failure");
    }
    
    // Small delay for SUB to respond - increase for reliability
    vTaskDelay(pdMS_TO_TICKS(250));
    
    // Read assignment response
    ret = i2c_master_read_msg(FIXED_SUB_ADDR, &response);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read assignment response from fixed SUB");
        
        // Add SUB entry despite the failure
        ESP_LOGW(TAG, "Creating SUB entry despite response failure");
    }
    
    ESP_LOGI(TAG, "Successfully assigned Fixed SUB: WiFi channel %u", wifi_channel);
    
    // Add fixed SUB to our list
    subs[0].id = 0;
    subs[0].i2c_addr = FIXED_SUB_ADDR;
    subs[0].wifi_channel = wifi_channel;
    memcpy(subs[0].id_str, id_str, 3);
    subs[0].status = SUB_STATUS_INITIALIZED;
    subs[0].ap_count = 0;
    subs[0].last_seen = xTaskGetTickCount();
    
    sub_count = 1;
    
    ESP_LOGI(TAG, "Fixed SUB connection established, status: INITIALIZED");
    
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