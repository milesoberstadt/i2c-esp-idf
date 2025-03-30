#include "wifi_scanner.h"
#include "ap_list.h"
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char *TAG = "wifi_scanner";

// Timestamp for recording when APs were seen
static uint32_t current_timestamp = 0;

// Current scanning state
static bool is_scanning = false;
static uint8_t current_channel = 0;

// Task handle for periodic probe requests
static TaskHandle_t probe_task_handle = NULL;

// Probe request interval (milliseconds)
#define PROBE_INTERVAL_MS 5000

// WiFi sniffer packet types we're interested in
#define WIFI_SNIFFER_PACKET_TYPE (WIFI_PKT_MGMT | WIFI_PKT_DATA | WIFI_PKT_CTRL)

// Probe request task
static void probe_request_task(void *pvParameters) {
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (is_scanning) {
        // Send probe request
        wifi_scanner_send_probe_request();
        
        // Wait for the next interval
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(PROBE_INTERVAL_MS));
    }
    
    vTaskDelete(NULL);
}

// WiFi sniffer callback for capturing packets
static void wifi_sniffer_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!buf || !is_scanning) return;
    
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    wifi_pkt_rx_ctrl_t *rx_ctrl = &pkt->rx_ctrl;
    
    // Check if the packet is a management frame (beacon, probe response)
    if (type == WIFI_PKT_MGMT) {
        const uint8_t *frame = pkt->payload;
        const uint16_t frame_len = pkt->rx_ctrl.sig_len;
        
        // Check if it's a beacon (Type 0, Subtype 8) or probe response (Type 0, Subtype 5)
        const uint8_t frame_type = frame[0] & 0x0C;
        const uint8_t frame_subtype = (frame[0] & 0xF0) >> 4;
        
        if (frame_type == 0 && (frame_subtype == 8 || frame_subtype == 5)) {
            if (frame_len < 36) {
                return; // Too short for a valid beacon or probe response
            }
            
            // Extract BSSID (source address)
            uint8_t bssid[6];
            memcpy(bssid, &frame[10], 6);
            
            // Extract SSID
            const uint8_t *ssid_section = &frame[36];
            
            // Check if we have a valid SSID element (ID 0)
            if (ssid_section[0] == 0) {
                uint8_t ssid_len = ssid_section[1];
                const uint8_t *ssid = &ssid_section[2];
                
                // Add or update AP in our list
                ap_list_add_or_update(
                    bssid,
                    ssid,
                    ssid_len,
                    rx_ctrl->rssi,
                    rx_ctrl->channel,
                    current_timestamp
                );
            }
        }
        // EAPOL packet detection (Type 2, Subtype 8 - QoS Data)
        else if (frame_type == 2 && frame_subtype == 8 && frame_len > 24) {
            // EAPOL detection logic - look for EAPOL signature
            // 0x888E in the LLC/SNAP header
            if (frame_len >= 34 && frame[30] == 0x88 && frame[31] == 0x8E) {
                ESP_LOGI(TAG, "EAPOL packet detected! RSSI: %d", rx_ctrl->rssi);
                
                // TODO: Store EAPOL packet for later retrieval
                // For now, just log detection
            }
        }
    }
}

esp_err_t wifi_scanner_init(void) {
    ESP_LOGI(TAG, "Initializing WiFi scanner");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // Initialize the AP list
    ap_list_init();
    
    current_timestamp = 0;
    is_scanning = false;
    current_channel = 0;
    
    ESP_LOGI(TAG, "WiFi scanner initialized");
    return ESP_OK;
}

esp_err_t wifi_scanner_start(uint8_t channel) {
    if (is_scanning) {
        ESP_LOGW(TAG, "WiFi scanner already running on channel %d", current_channel);
        
        // If channel is the same, just return
        if (channel == current_channel) {
            return ESP_OK;
        }
        
        // Otherwise, stop current scanning
        wifi_scanner_stop();
    }
    
    ESP_LOGI(TAG, "Starting WiFi scanner on FIXED channel %d", channel);
    
    // Set the WiFi channel
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
    
    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Set promiscuous mode
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&(wifi_promiscuous_filter_t){
        .filter_mask = WIFI_SNIFFER_PACKET_TYPE
    }));
    
    // Record current channel
    current_channel = channel;
    is_scanning = true;
    
    // Create task to send periodic probe requests
    xTaskCreate(probe_request_task, "probe_req", 2048, NULL, 5, &probe_task_handle);
    
    ESP_LOGI(TAG, "WiFi scanner started on fixed channel %d (all SUBs use the same channel)", channel);
    return ESP_OK;
}

esp_err_t wifi_scanner_stop(void) {
    if (!is_scanning) {
        ESP_LOGW(TAG, "WiFi scanner not running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping WiFi scanner");
    
    // Signal task to stop and wait for completion
    is_scanning = false;
    if (probe_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Give the task time to terminate
        probe_task_handle = NULL;
    }
    
    // Disable promiscuous mode
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    
    // Stop WiFi
    ESP_ERROR_CHECK(esp_wifi_stop());
    
    // Reset state
    current_channel = 0;
    
    ESP_LOGI(TAG, "WiFi scanner stopped");
    return ESP_OK;
}

// This is a simplified implementation - a real implementation would need to craft
// a proper probe request frame according to 802.11 standards
esp_err_t wifi_scanner_send_probe_request(void) {
    if (!is_scanning) {
        ESP_LOGW(TAG, "Cannot send probe request - WiFi scanner not running");
        return ESP_ERR_INVALID_STATE;
    }
    
    // TODO: In a real implementation, craft and send an actual probe request frame
    // For now, we'll just log that we would send one
    ESP_LOGD(TAG, "Sending probe request on channel %d", current_channel);
    
    return ESP_OK;
}

void wifi_scanner_set_timestamp(uint32_t timestamp) {
    ESP_LOGI(TAG, "Setting timestamp to %" PRIu32, timestamp);
    current_timestamp = timestamp;
}

uint32_t wifi_scanner_get_timestamp(void) {
    return current_timestamp;
}