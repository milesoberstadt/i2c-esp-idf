#include "ap_list.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "ap_list";

// The AP record list
static ap_record_t ap_list[MAX_AP_LIST_SIZE];
static uint16_t ap_count = 0;

void ap_list_init(void) {
    // Reset the list
    memset(ap_list, 0, sizeof(ap_list));
    ap_count = 0;
    ESP_LOGI(TAG, "AP list initialized");
}

// Compare two BSSIDs
static bool bssid_equal(const uint8_t *bssid1, const uint8_t *bssid2) {
    return memcmp(bssid1, bssid2, 6) == 0;
}

// Find an AP record by BSSID
static int16_t ap_list_find_by_bssid(const uint8_t *bssid) {
    for (uint16_t i = 0; i < ap_count; i++) {
        if (bssid_equal(ap_list[i].bssid, bssid)) {
            return i;
        }
    }
    return -1; // Not found
}

bool ap_list_add_or_update(const uint8_t *bssid, const uint8_t *ssid, 
                          uint8_t ssid_len, int8_t rssi, uint8_t channel, 
                          uint32_t timestamp) {
    int16_t idx = ap_list_find_by_bssid(bssid);
    
    // If found, check if we should update based on signal strength
    if (idx >= 0) {
        // Only update if new signal is stronger
        if (rssi > ap_list[idx].rssi) {
            ap_list[idx].rssi = rssi;
            ap_list[idx].timestamp = timestamp;
            ap_list[idx].synced = false;
            ESP_LOGD(TAG, "Updated AP: RSSI %d", rssi);
            return true;
        }
        return false; // No update needed
    }
    
    // Not found, add new entry if space available
    if (ap_count < MAX_AP_LIST_SIZE) {
        // Copy BSSID
        memcpy(ap_list[ap_count].bssid, bssid, 6);
        
        // Copy SSID (ensure null-terminated)
        if (ssid_len > 32) ssid_len = 32;
        memcpy(ap_list[ap_count].ssid, ssid, ssid_len);
        ap_list[ap_count].ssid[ssid_len] = '\0';
        
        // Set other fields
        ap_list[ap_count].rssi = rssi;
        ap_list[ap_count].channel = channel;
        ap_list[ap_count].timestamp = timestamp;
        ap_list[ap_count].synced = false;
        
        ap_count++;
        ESP_LOGD(TAG, "Added new AP (%d): RSSI %d, channel %d", 
                 ap_count, rssi, channel);
        return true;
    } else {
        ESP_LOGW(TAG, "AP list full, can't add new AP");
        
        // Find the weakest signal AP and replace if this one is stronger
        int16_t weakest_idx = 0;
        int8_t weakest_rssi = ap_list[0].rssi;
        
        for (uint16_t i = 1; i < ap_count; i++) {
            if (ap_list[i].rssi < weakest_rssi) {
                weakest_rssi = ap_list[i].rssi;
                weakest_idx = i;
            }
        }
        
        // Replace the weakest if this signal is stronger
        if (rssi > weakest_rssi) {
            ESP_LOGI(TAG, "Replacing weak AP (RSSI: %d) with stronger one (RSSI: %d)", 
                     weakest_rssi, rssi);
            
            // Copy BSSID
            memcpy(ap_list[weakest_idx].bssid, bssid, 6);
            
            // Copy SSID (ensure null-terminated)
            if (ssid_len > 32) ssid_len = 32;
            memcpy(ap_list[weakest_idx].ssid, ssid, ssid_len);
            ap_list[weakest_idx].ssid[ssid_len] = '\0';
            
            // Set other fields
            ap_list[weakest_idx].rssi = rssi;
            ap_list[weakest_idx].channel = channel;
            ap_list[weakest_idx].timestamp = timestamp;
            ap_list[weakest_idx].synced = false;
            
            return true;
        }
        
        return false; // No update performed
    }
}

uint16_t ap_list_get_count(void) {
    return ap_count;
}

bool ap_list_get_next_unsync(ap_record_t *record) {
    if (!record) return false;
    
    for (uint16_t i = 0; i < ap_count; i++) {
        if (!ap_list[i].synced) {
            memcpy(record, &ap_list[i], sizeof(ap_record_t));
            return true;
        }
    }
    
    return false; // No unsynced records found
}

bool ap_list_mark_synced(const uint8_t *bssid) {
    int16_t idx = ap_list_find_by_bssid(bssid);
    if (idx >= 0) {
        ap_list[idx].synced = true;
        return true;
    }
    return false;
}

void ap_list_reset_sync_flags(void) {
    for (uint16_t i = 0; i < ap_count; i++) {
        ap_list[i].synced = false;
    }
    ESP_LOGI(TAG, "Reset sync flags for all AP records");
}

bool ap_list_get_by_index(uint16_t index, ap_record_t *record) {
    if (!record || index >= ap_count) return false;
    
    memcpy(record, &ap_list[index], sizeof(ap_record_t));
    return true;
}

void ap_list_clear(void) {
    memset(ap_list, 0, sizeof(ap_list));
    ap_count = 0;
    ESP_LOGI(TAG, "AP list cleared");
}