#ifndef AP_DATABASE_H
#define AP_DATABASE_H

#include <stdbool.h>
#include <stdint.h>
#include "types.h"

// Initialize the AP database
void ap_database_init(void);

// Add or update an AP record in the database
bool ap_database_add(const ap_record_t *record);

// Get the total number of APs in the database
uint32_t ap_database_get_count(void);

// Get an AP record by index
bool ap_database_get_by_index(uint32_t index, ap_record_t *record);

// Search for an AP by BSSID
bool ap_database_find_by_bssid(const uint8_t *bssid, ap_record_t *record);

// Clear the entire database
void ap_database_clear(void);

#endif // AP_DATABASE_H