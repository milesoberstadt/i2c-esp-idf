#ifndef __WIFI_SNIFFER_H__
#define __WIFI_SNIFFER_H__

#include <stdbool.h>
#include <stdint.h>
#include "esp_event.h"
#include "esp_wifi_types.h"

ESP_EVENT_DECLARE_BASE(SNIFFER_EVENTS);

enum {
    SNIFFER_EVENT_CAPTURED_DATA,
    SNIFFER_EVENT_CAPTURED_MGMT,
    SNIFFER_EVENT_CAPTURED_CTRL,
    SNIFFER_EVENT_BEACON
};

// Stats structure to track sniffing results
typedef struct {
    uint32_t packet_count;
    uint32_t beacon_count;
    uint32_t data_count;
    uint32_t mgmt_count;
    uint32_t ctrl_count;
} wifi_stats_t;

/**
 * @brief Initialize WiFi in station mode for sniffing
 * 
 * @return true if initialization was successful, false otherwise
 */
bool wifi_sniffer_init(void);

/**
 * @brief Sets sniffer filter for beacon frames only
 */
void wifi_sniffer_filter_beacons_only(void);

/**
 * @brief Sets sniffer filter for specific frame types
 * 
 * @param data sniff data frames
 * @param mgmt sniff management frames
 * @param ctrl sniff control frames
 */
void wifi_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl);

/**
 * @brief Start promiscuous mode on given channel
 * 
 * @param channel channel on which sniffer should operate (1-14)
 * @return true if sniffer was started successfully, false otherwise
 */
bool wifi_sniffer_start(uint8_t channel);

/**
 * @brief Stop promiscuous mode
 * 
 * @return true if sniffer was stopped successfully, false otherwise
 */
bool wifi_sniffer_stop(void);

/**
 * @brief Set WiFi channel for sniffing
 * 
 * @param channel channel on which sniffer should operate (1-14)
 * @return true if channel was set successfully, false otherwise
 */
bool wifi_sniffer_set_channel(uint8_t channel);

/**
 * @brief Get current WiFi sniffer statistics
 * 
 * @return wifi_stats_t structure with packet counts
 */
wifi_stats_t wifi_sniffer_get_stats(void);

/**
 * @brief Reset WiFi sniffer statistics
 */
void wifi_sniffer_reset_stats(void);

/**
 * @brief Event handler for WiFi sniffer events
 */
void sniffer_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data);

/**
 * @brief Task to handle WiFi sniffer events
 */
void wifi_sniffer_event_handler_task(void *pvParameters);

#endif // __WIFI_SNIFFER_H__