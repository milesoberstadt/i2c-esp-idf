#include "ap_database.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "ap_database";

// Maximum number of APs to store
#define MAX_AP_COUNT 2000

// AP record database
static ap_record_t *ap_records = NULL;
static uint32_t ap_count = 0;

// Mutex for thread safety
static SemaphoreHandle_t db_mutex = NULL;

void ap_database_init(void) {
    ESP_LOGI(TAG, "Initializing AP database");
    
    // Create mutex
    db_mutex = xSemaphoreCreateMutex();
    if (db_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }
    
    // Allocate memory for records
    ap_records = calloc(MAX_AP_COUNT, sizeof(ap_record_t));
    if (ap_records == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for AP database");
        return;
    }
    
    ap_count = 0;
    
    ESP_LOGI(TAG, "AP database initialized with capacity for %lu APs", (unsigned long)MAX_AP_COUNT);
}

// Compare two BSSIDs
static bool bssid_equal(const uint8_t *bssid1, const uint8_t *bssid2) {
    return memcmp(bssid1, bssid2, 6) == 0;
}

// Find an AP record by BSSID (internal, no locking)
static int32_t find_ap_by_bssid(const uint8_t *bssid) {
    for (uint32_t i = 0; i < ap_count; i++) {
        if (bssid_equal(ap_records[i].bssid, bssid)) {
            return i;
        }
    }
    return -1; // Not found
}

bool ap_database_add(const ap_record_t *record) {
    if (!record) return false;
    
    if (xSemaphoreTake(db_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return false;
    }
    
    bool result = false;
    
    // Check if AP already exists
    int32_t idx = find_ap_by_bssid(record->bssid);
    
    if (idx >= 0) {
        // AP exists, update if new signal is stronger
        if (record->rssi > ap_records[idx].rssi) {
            ap_records[idx].rssi = record->rssi;
            ap_records[idx].timestamp = record->timestamp;
            ap_records[idx].channel = record->channel;
            ESP_LOGD(TAG, "Updated AP: RSSI %d", record->rssi);
            result = true;
        } else {
            result = false; // No update needed
        }
    } else {
        // AP not found, add new if space available
        if (ap_count < MAX_AP_COUNT) {
            // Copy record
            memcpy(&ap_records[ap_count], record, sizeof(ap_record_t));
            
            // Set synced flag to true (already synced to DOM)
            ap_records[ap_count].synced = true;
            
            ap_count++;
            ESP_LOGD(TAG, "Added new AP (total: %lu): RSSI %d, channel %u", 
                     (unsigned long)ap_count, record->rssi, record->channel);
            result = true;
        } else {
            ESP_LOGW(TAG, "AP database full, can't add new AP");
            result = false;
        }
    }
    
    xSemaphoreGive(db_mutex);
    return result;
}

uint32_t ap_database_get_count(void) {
    if (xSemaphoreTake(db_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return 0;
    }
    
    uint32_t count = ap_count;
    
    xSemaphoreGive(db_mutex);
    return count;
}

bool ap_database_get_by_index(uint32_t index, ap_record_t *record) {
    if (!record) return false;
    
    if (xSemaphoreTake(db_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return false;
    }
    
    bool result = false;
    
    if (index < ap_count) {
        memcpy(record, &ap_records[index], sizeof(ap_record_t));
        result = true;
    } else {
        result = false;
    }
    
    xSemaphoreGive(db_mutex);
    return result;
}

bool ap_database_find_by_bssid(const uint8_t *bssid, ap_record_t *record) {
    if (!bssid || !record) return false;
    
    if (xSemaphoreTake(db_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return false;
    }
    
    bool result = false;
    
    int32_t idx = find_ap_by_bssid(bssid);
    if (idx >= 0) {
        memcpy(record, &ap_records[idx], sizeof(ap_record_t));
        result = true;
    } else {
        result = false;
    }
    
    xSemaphoreGive(db_mutex);
    return result;
}

void ap_database_clear(void) {
    if (xSemaphoreTake(db_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return;
    }
    
    memset(ap_records, 0, sizeof(ap_record_t) * MAX_AP_COUNT);
    ap_count = 0;
    
    ESP_LOGI(TAG, "AP database cleared");
    
    xSemaphoreGive(db_mutex);
}