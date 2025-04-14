#include "wifi_sniffer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_sniffer";

ESP_EVENT_DEFINE_BASE(SNIFFER_EVENTS);

/**
 * @brief Checks if a frame is a beacon frame
 * 
 * @param payload The frame payload
 * @return true if it's a beacon frame, false otherwise
 */
static bool is_beacon_frame(const uint8_t *payload) {
    // Check if it's a beacon frame (management frame type 0, subtype 8)
    // Type is in bits 2-3, subtype in bits 4-7 of the frame control field
    uint16_t frame_control = payload[0] | (payload[1] << 8);
    
    uint8_t frame_type = (frame_control >> 2) & 0x3;     // Bits 2-3
    uint8_t frame_subtype = (frame_control >> 4) & 0xF;  // Bits 4-7
    
    return (frame_type == 0 && frame_subtype == 8);  // Type 0 = Management, Subtype 8 = Beacon
}

/**
 * @brief Callback for promiscuous receiver
 * 
 * It forwards captured frames into event pool and sorts them based on their type.
 * Also detects beacon frames and posts them as a separate event.
 * 
 * @param buf Buffer containing the frame
 * @param type The type of the frame
 */
static void frame_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    ESP_LOGV(TAG, "Captured frame %d", (int) type);

    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) buf;
    const uint8_t *payload = frame->payload;
    
    int32_t event_id;
    switch (type) {
        case WIFI_PKT_DATA:
            event_id = SNIFFER_EVENT_CAPTURED_DATA;
            break;
        case WIFI_PKT_MGMT:
            event_id = SNIFFER_EVENT_CAPTURED_MGMT;
            
            // Check if this is a beacon frame and post a special event
            if (is_beacon_frame(payload)) {
                ESP_LOGI(TAG, "Beacon frame detected");
                // Extract and print the SSID from the beacon frame
                const uint8_t ssid_length = payload[37];       // SSID length is at offset 37
                const char *ssid = (const char *)&payload[38]; // SSID starts at offset 38
                ESP_LOGI(TAG, "SSID: %.*s", ssid_length, ssid);

                ESP_ERROR_CHECK(esp_event_post(SNIFFER_EVENTS, SNIFFER_EVENT_BEACON, 
                                             frame, frame->rx_ctrl.sig_len + sizeof(wifi_promiscuous_pkt_t), 
                                             portMAX_DELAY));
                return;  // Skip normal event post for beacons to avoid duplication
            }
            break;
        case WIFI_PKT_CTRL:
            event_id = SNIFFER_EVENT_CAPTURED_CTRL;
            break;
        default:
            return;
    }

    ESP_ERROR_CHECK(esp_event_post(SNIFFER_EVENTS, event_id, 
                                 frame, frame->rx_ctrl.sig_len + sizeof(wifi_promiscuous_pkt_t), 
                                 portMAX_DELAY));
}

void wifi_sniffer_init(void) {
    ESP_LOGI(TAG, "Initializing WiFi in station mode");
    
    // Initialize TCP/IP adapter
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Create default WiFi STA netif
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    
    // Initialize WiFi
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    
    // Configure WiFi mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi initialized in station mode");
}

void wifi_sniffer_filter_beacons_only(void) {
    ESP_LOGI(TAG, "Setting sniffer filter for beacon frames only");
    
    // Configure filter for management frames only (beacons are management frames)
    wifi_promiscuous_filter_t filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
}

void wifi_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl) {
    ESP_LOGI(TAG, "Setting sniffer filter - data: %d, mgmt: %d, ctrl: %d", data, mgmt, ctrl);
    
    wifi_promiscuous_filter_t filter = { .filter_mask = 0 };
    
    // Match the original code's behavior which uses else-if
    if(data) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
    }
    else if(mgmt) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;
    }
    else if(ctrl) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL;
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
}

void wifi_sniffer_start(uint8_t channel) {
    ESP_LOGI(TAG, "Starting promiscuous mode on channel %d", (int) channel);
    
    // Set channel
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
    
    // Register callback and enable promiscuous mode
    esp_wifi_set_promiscuous_rx_cb(&frame_handler);
    esp_wifi_set_promiscuous(true);
    
    ESP_LOGI(TAG, "Promiscuous mode started");
}

void wifi_sniffer_stop(void) {
    ESP_LOGI(TAG, "Stopping promiscuous mode");
    esp_wifi_set_promiscuous(false);
}

/**
 * @brief Task to handle WiFi sniffer events
 *
 * This task registers for all sniffer events and processes them
 */
void wifi_sniffer_event_handler_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting WiFi sniffer event handler task");
    // Initialize WiFi in station mode for sniffing
    wifi_sniffer_init();

    // Create event loop for handling WiFi events
    esp_event_loop_handle_t event_loop_handle;
    esp_event_loop_args_t event_loop_args = {
        .queue_size = 10,
        .task_name = "sniffer_event_loop",
        .task_priority = 5,
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY};

    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &event_loop_handle));

    // Register for all sniffer events
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        event_loop_handle,
        SNIFFER_EVENTS,
        ESP_EVENT_ANY_ID,
        sniffer_event_handler,
        NULL));

    ESP_LOGI(TAG, "Registered for sniffer events");

    // Keep task alive
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Event handler for WiFi sniffer events
 */
void sniffer_event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data) {
    if (event_base != SNIFFER_EVENTS)
    {
        return;
    }

    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *)event_data;

    switch (event_id)
    {
    case SNIFFER_EVENT_BEACON:
        ESP_LOGI(TAG, "Beacon frame detected: RSSI=%d, Channel=%d",
                 (int) frame->rx_ctrl.rssi,
                 (int) frame->rx_ctrl.channel);

        // For now, we're just logging beacon detections
        // In the future, this is where we could send beacon data to the DOM node via I2C
        break;

    case SNIFFER_EVENT_CAPTURED_MGMT:
        ESP_LOGD(TAG, "Management frame detected");
        break;

    case SNIFFER_EVENT_CAPTURED_DATA:
        ESP_LOGD(TAG, "Data frame detected");
        break;

    case SNIFFER_EVENT_CAPTURED_CTRL:
        ESP_LOGD(TAG, "Control frame detected");
        break;

    default:
        ESP_LOGW(TAG, "Unknown sniffer event: %" PRId32, event_id);
        break;
    }
}