#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include "esp_err.h"
#include "esp_wifi_types.h"

// Initialize WiFi in promiscuous mode
esp_err_t wifi_scanner_init(void);

// Start WiFi scanning on the specified channel
esp_err_t wifi_scanner_start(uint8_t channel);

// Stop WiFi scanning
esp_err_t wifi_scanner_stop(void);

// Send a probe request (for active scanning)
esp_err_t wifi_scanner_send_probe_request(void);

// Set the current timestamp (from DOM node)
void wifi_scanner_set_timestamp(uint32_t timestamp);

// Get the current timestamp
uint32_t wifi_scanner_get_timestamp(void);

#endif // WIFI_SCANNER_H