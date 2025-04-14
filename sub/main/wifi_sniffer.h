#ifndef __WIFI_SNIFFER_H__
#define __WIFI_SNIFFER_H__

#include <stdbool.h>
#include "esp_event.h"
#include "esp_wifi_types.h"

ESP_EVENT_DECLARE_BASE(SNIFFER_EVENTS);

enum {
    SNIFFER_EVENT_CAPTURED_DATA,
    SNIFFER_EVENT_CAPTURED_MGMT,
    SNIFFER_EVENT_CAPTURED_CTRL,
    SNIFFER_EVENT_BEACON
};

/**
 * @brief Initialize WiFi in station mode for sniffing
 */
void wifi_sniffer_init(void);

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
 */
void wifi_sniffer_start(uint8_t channel);

/**
 * @brief Stop promiscuous mode
 */
void wifi_sniffer_stop(void);

#endif // __WIFI_SNIFFER_H__