#ifndef AP_LIST_H
#define AP_LIST_H

#include "types.h"
#include "esp_wifi_types.h"

// Initialize the AP list
void ap_list_init(void);

// Add or update an AP record in the list
bool ap_list_add_or_update(const uint8_t *bssid, const uint8_t *ssid, 
                          uint8_t ssid_len, int8_t rssi, uint8_t channel, 
                          uint32_t timestamp);

// Get the total number of AP records
uint16_t ap_list_get_count(void);

// Get the next unsynchronized AP record
bool ap_list_get_next_unsync(ap_record_t *record);

// Mark an AP record as synchronized
bool ap_list_mark_synced(const uint8_t *bssid);

// Reset all sync flags (set all to unsynced)
void ap_list_reset_sync_flags(void);

// Get an AP record by index
bool ap_list_get_by_index(uint16_t index, ap_record_t *record);

// Clear the entire AP list
void ap_list_clear(void);

#endif // AP_LIST_H