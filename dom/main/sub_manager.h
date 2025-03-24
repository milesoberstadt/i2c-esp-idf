#ifndef SUB_MANAGER_H
#define SUB_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "types.h"

// Initialize the SUB manager
esp_err_t sub_manager_init(void);

// Discover SUBs on the I2C bus
esp_err_t sub_manager_discover(void);

// Set timestamp on all SUBs
esp_err_t sub_manager_set_timestamp(uint32_t timestamp);

// Start scanning on all SUBs
esp_err_t sub_manager_start_scanning(void);

// Stop scanning on all SUBs
esp_err_t sub_manager_stop_scanning(void);

// Synchronize AP data from all SUBs
esp_err_t sub_manager_sync_ap_data(void);

// Get the number of discovered SUBs
uint8_t sub_manager_get_count(void);

// Get SUB info by index
sub_info_t *sub_manager_get_sub(uint8_t index);

// Get total AP count across all SUBs
uint32_t sub_manager_get_total_ap_count(void);

// Reset all SUBs
esp_err_t sub_manager_reset_all(void);

// Check if SUB manager is busy
bool sub_manager_is_busy(void);

#endif // SUB_MANAGER_H