#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/spi_slave.h"
#include "driver/gpio.h"

#define MAIN_TAG "SUB_MAIN"
#define SPI_TAG "SUB_SPI"
#define WIFI_TAG "SUB_WIFI"

// SPI pin definitions based on README.md (SUB node pins)
#define PIN_NUM_MISO 8
#define PIN_NUM_MOSI 9
#define PIN_NUM_CLK  7
#define PIN_NUM_CS   1

// SPI configuration
#define SPI_HOST_ID  SPI2_HOST
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define SPI_BUFFER_SIZE 64

// Communication protocol
#define CMD_GET_WIFI_COUNT 0x01
#define CMD_ASSIGN_CHANNEL 0x02

// WiFi scanning
#define MAX_AP_RECORDS 500

typedef struct {
  uint64_t first_seen_time;
  int8_t rssi;
  char ssid[33];
  uint8_t bssid[6];
} ap_record_t;

// WiFi frame structures for promiscuous mode
typedef struct {
  int16_t fctl;
  int16_t duration;
  uint8_t da[6];      // destination address
  uint8_t sa[6];      // source address
  uint8_t bssid[6];   // BSSID
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) wifi_mgmt_hdr_t;

// Frame control field values
#define FRAME_TYPE_BEACON 0x8000

// Global variables
static ap_record_t ap_records[MAX_AP_RECORDS];
static uint32_t ap_count = 0;
static SemaphoreHandle_t ap_list_mutex;
static uint8_t spi_rx_buffer[SPI_BUFFER_SIZE];
static uint8_t spi_tx_buffer[SPI_BUFFER_SIZE];
static bool spi_comm_established = false;
static uint8_t assigned_channel = 0;  // 0 means not assigned yet
static bool channel_assigned = false;  // true when DOM has assigned a channel

// Forward declarations
void add_ap_record(wifi_ap_record_t *ap_info);

esp_err_t wifi_init(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  
  ESP_LOGI(WIFI_TAG, "WiFi initialized for scanning");
  return ESP_OK;
}

// Parse SSID from beacon frame tagged parameters
// Beacon frame structure: MAC header (24) + Fixed parameters (12) + Tagged parameters
// SSID is tag 0 with format: [tag_num(1)][length(1)][ssid_data(length)]
bool parse_beacon_ssid(const uint8_t *payload, uint16_t len, char *ssid_out, uint8_t *ssid_len_out) {
  // Fixed parameters start at offset 0 in payload (after MAC header)
  // Tagged parameters start at offset 12 (timestamp 8 + beacon_interval 2 + capability 2)
  const uint8_t FIXED_PARAMS_LEN = 12;

  if (len < FIXED_PARAMS_LEN + 2) {
    return false;  // Too short to contain any tagged parameters
  }

  const uint8_t *tagged_params = payload + FIXED_PARAMS_LEN;
  uint16_t remaining = len - FIXED_PARAMS_LEN;

  // Search through tagged parameters for SSID (tag 0)
  while (remaining >= 2) {
    uint8_t tag_num = tagged_params[0];
    uint8_t tag_len = tagged_params[1];

    if (remaining < (2 + tag_len)) {
      break;  // Malformed frame
    }

    if (tag_num == 0) {  // SSID tag
      if (tag_len > 32) tag_len = 32;  // Limit SSID length
      memcpy(ssid_out, &tagged_params[2], tag_len);
      ssid_out[tag_len] = '\0';
      *ssid_len_out = tag_len;
      return true;
    }

    // Move to next tag
    tagged_params += (2 + tag_len);
    remaining -= (2 + tag_len);
  }

  return false;  // SSID tag not found
}

// Promiscuous mode packet handler for beacon frames
void wifi_promiscuous_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
  // Only process management frames
  if (type != WIFI_PKT_MGMT) {
    return;
  }

  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  wifi_mgmt_hdr_t *mgmt = (wifi_mgmt_hdr_t *)pkt->payload;

  // Check frame control field for beacon frames
  uint16_t fc = (mgmt->fctl);

  // Beacon frame: type=0 (management), subtype=8 (beacon) = 0x0080 in little-endian
  // After byte swap it becomes 0x8000
  if ((fc & 0xFF00) != 0x8000) {
    return;  // Not a beacon frame
  }

  // Extract SSID from beacon frame
  char ssid[33] = {0};
  uint8_t ssid_len = 0;

  // Payload starts after MAC header (24 bytes are in wifi_mgmt_hdr_t up to seqctl)
  // The payload field contains the beacon frame body
  if (!parse_beacon_ssid(mgmt->payload, pkt->rx_ctrl.sig_len - sizeof(wifi_mgmt_hdr_t) + sizeof(unsigned char *), ssid, &ssid_len)) {
    return;  // Failed to parse SSID
  }

  // Skip hidden SSIDs (length 0) or empty SSIDs
  if (ssid_len == 0) {
    return;
  }

  // Create a wifi_ap_record_t structure for compatibility with existing storage
  wifi_ap_record_t ap_info;
  memset(&ap_info, 0, sizeof(ap_info));
  memcpy(ap_info.ssid, ssid, ssid_len);
  ap_info.ssid[ssid_len] = '\0';
  memcpy(ap_info.bssid, mgmt->bssid, 6);
  ap_info.rssi = pkt->rx_ctrl.rssi;

  // Store the AP record
  add_ap_record(&ap_info);
}

void add_ap_record(wifi_ap_record_t *ap_info) {
  if (xSemaphoreTake(ap_list_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    // Check if AP already exists
    for (uint32_t i = 0; i < ap_count; i++) {
      if (memcmp(ap_records[i].bssid, ap_info->bssid, 6) == 0) {
        // Update RSSI if stronger signal
        if (ap_info->rssi > ap_records[i].rssi) {
          ap_records[i].rssi = ap_info->rssi;
          ESP_LOGD(WIFI_TAG, "Updated RSSI for %s: %d", (char*)ap_info->ssid, ap_info->rssi);
        }
        xSemaphoreGive(ap_list_mutex);
        return;
      }
    }

    // Add new AP
    if (ap_count < MAX_AP_RECORDS) {
      // Normal case: add to list
      ap_records[ap_count].first_seen_time = esp_timer_get_time();
      ap_records[ap_count].rssi = ap_info->rssi;
      memcpy(ap_records[ap_count].ssid, ap_info->ssid, sizeof(ap_records[ap_count].ssid));
      memcpy(ap_records[ap_count].bssid, ap_info->bssid, 6);
      ap_count++;
      ESP_LOGI(WIFI_TAG, "Added AP %s (RSSI: %d) - Total: %lu",
               (char*)ap_info->ssid, ap_info->rssi, ap_count);
    } else {
      // List is full: remove oldest record and add new one
      ESP_LOGW(WIFI_TAG, "AP list full (500 records), removing oldest entry");

      // Find oldest record (lowest first_seen_time)
      uint32_t oldest_idx = 0;
      uint64_t oldest_time = ap_records[0].first_seen_time;
      for (uint32_t i = 1; i < ap_count; i++) {
        if (ap_records[i].first_seen_time < oldest_time) {
          oldest_time = ap_records[i].first_seen_time;
          oldest_idx = i;
        }
      }

      // Replace oldest record with new AP
      ap_records[oldest_idx].first_seen_time = esp_timer_get_time();
      ap_records[oldest_idx].rssi = ap_info->rssi;
      memcpy(ap_records[oldest_idx].ssid, ap_info->ssid, sizeof(ap_records[oldest_idx].ssid));
      memcpy(ap_records[oldest_idx].bssid, ap_info->bssid, 6);

      ESP_LOGI(WIFI_TAG, "Replaced oldest AP with %s (RSSI: %d)",
               (char*)ap_info->ssid, ap_info->rssi);
    }

    xSemaphoreGive(ap_list_mutex);
  }
}

void wifi_scan_task(void *pvParameters) {
  ESP_LOGI(WIFI_TAG, "WiFi promiscuous mode task waiting for channel assignment from DOM node...");

  // Wait for channel assignment from DOM node before starting promiscuous mode
  while (!channel_assigned) {
    vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
  }

  ESP_LOGI(WIFI_TAG, "Channel %d assigned by DOM, starting promiscuous mode on channel %d",
           assigned_channel, assigned_channel);

  // Set the WiFi channel
  esp_err_t ret = esp_wifi_set_channel(assigned_channel, WIFI_SECOND_CHAN_NONE);
  if (ret != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "Failed to set WiFi channel %d: %s", assigned_channel, esp_err_to_name(ret));
    vTaskDelete(NULL);
    return;
  }

  // Register promiscuous mode callback
  ret = esp_wifi_set_promiscuous_rx_cb(wifi_promiscuous_packet_handler);
  if (ret != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "Failed to set promiscuous callback: %s", esp_err_to_name(ret));
    vTaskDelete(NULL);
    return;
  }

  // Enable promiscuous mode
  ret = esp_wifi_set_promiscuous(true);
  if (ret != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "Failed to enable promiscuous mode: %s", esp_err_to_name(ret));
    vTaskDelete(NULL);
    return;
  }

  ESP_LOGI(WIFI_TAG, "Promiscuous mode enabled on channel %d, listening for beacon frames...", assigned_channel);

  // Promiscuous mode runs in the background via callback
  // This task just keeps running and logs status periodically
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(30000)); // Log status every 30 seconds

    uint32_t current_count = 0;
    if (xSemaphoreTake(ap_list_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      current_count = ap_count;
      xSemaphoreGive(ap_list_mutex);
    }

    ESP_LOGI(WIFI_TAG, "Promiscuous mode active on channel %d, %lu APs discovered",
             assigned_channel, current_count);
  }
}

esp_err_t spi_init(void) {
  spi_bus_config_t buscfg = {
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = SPI_BUFFER_SIZE,
  };

  spi_slave_interface_config_t slvcfg = {
    .mode = 0,
    .spics_io_num = PIN_NUM_CS,
    .queue_size = 3,
    .flags = 0,
  };

  esp_err_t ret = spi_slave_initialize(SPI_HOST_ID, &buscfg, &slvcfg, SPI_DMA_CHAN);
  if (ret != ESP_OK) {
    ESP_LOGE(SPI_TAG, "Failed to initialize SPI slave: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(SPI_TAG, "SPI slave initialized successfully");
  return ESP_OK;
}

void spi_slave_task(void *pvParameters) {
  ESP_LOGI(SPI_TAG, "Starting SPI slave task, waiting for DOM communication...");
  uint8_t last_cmd = 0x00;  // Track last received command

  while (1) {
    memset(spi_rx_buffer, 0, SPI_BUFFER_SIZE);
    memset(spi_tx_buffer, 0, SPI_BUFFER_SIZE);

    // Prepare TX buffer BEFORE transaction based on last command received
    if (last_cmd == CMD_ASSIGN_CHANNEL) {
      // Last command was channel assignment, prepare acknowledgment
      spi_tx_buffer[0] = CMD_ASSIGN_CHANNEL;
      spi_tx_buffer[1] = assigned_channel;  // Echo back the assigned channel
    } else {
      // Default response: current WiFi count
      uint32_t current_count = 0;
      if (xSemaphoreTake(ap_list_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        current_count = ap_count;
        xSemaphoreGive(ap_list_mutex);
      }

      spi_tx_buffer[0] = CMD_GET_WIFI_COUNT;
      spi_tx_buffer[1] = (current_count >> 24) & 0xFF;
      spi_tx_buffer[2] = (current_count >> 16) & 0xFF;
      spi_tx_buffer[3] = (current_count >> 8) & 0xFF;
      spi_tx_buffer[4] = current_count & 0xFF;
    }

    spi_slave_transaction_t trans = {
      .length = SPI_BUFFER_SIZE * 8,
      .rx_buffer = spi_rx_buffer,
      .tx_buffer = spi_tx_buffer,
    };

    esp_err_t ret = spi_slave_transmit(SPI_HOST_ID, &trans, portMAX_DELAY);
    if (ret == ESP_OK) {
      // Log SPI traffic
      ESP_LOGI(SPI_TAG, "SPI transaction completed");
      ESP_LOGI(SPI_TAG, "RX: %02X %02X %02X %02X %02X %02X %02X %02X",
               spi_rx_buffer[0], spi_rx_buffer[1], spi_rx_buffer[2], spi_rx_buffer[3],
               spi_rx_buffer[4], spi_rx_buffer[5], spi_rx_buffer[6], spi_rx_buffer[7]);
      ESP_LOGI(SPI_TAG, "TX: %02X %02X %02X %02X %02X %02X %02X %02X",
               spi_tx_buffer[0], spi_tx_buffer[1], spi_tx_buffer[2], spi_tx_buffer[3],
               spi_tx_buffer[4], spi_tx_buffer[5], spi_tx_buffer[6], spi_tx_buffer[7]);

      // Process received command
      uint8_t cmd = spi_rx_buffer[0];
      last_cmd = cmd;  // Remember for next iteration

      if (cmd == CMD_ASSIGN_CHANNEL) {
        // Channel assignment command
        uint8_t channel = spi_rx_buffer[1];
        ESP_LOGI(SPI_TAG, "Received channel assignment: channel %d", channel);

        // Store the assigned channel
        assigned_channel = channel;
        channel_assigned = true;

        if (!spi_comm_established) {
          spi_comm_established = true;
          ESP_LOGI(SPI_TAG, "SPI communication established with DOM node!");
        }

        ESP_LOGI(SPI_TAG, "Channel %d stored, will acknowledge in next transaction", channel);

      } else if (cmd == CMD_GET_WIFI_COUNT) {
        if (!spi_comm_established) {
          spi_comm_established = true;
          ESP_LOGI(SPI_TAG, "SPI communication established with DOM node!");
        }

        // Get current count for logging
        uint32_t current_count = 0;
        if (xSemaphoreTake(ap_list_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
          current_count = ap_count;
          xSemaphoreGive(ap_list_mutex);
        }
        ESP_LOGI(SPI_TAG, "Responded with WiFi count: %lu", current_count);

      } else if (cmd == 0x00) {
        ESP_LOGD(SPI_TAG, "Received empty command, likely DOM not ready yet");
      } else {
        ESP_LOGW(SPI_TAG, "Unknown command: 0x%02X", cmd);
      }

    } else {
      ESP_LOGE(SPI_TAG, "SPI slave transmit failed: %s", esp_err_to_name(ret));
    }
  }
}

void uptime_task(void *pvParameters) {
    while (1) {
        uint32_t uptime_ms = esp_timer_get_time() / 1000;
        uint32_t uptime_seconds = uptime_ms / 1000;
        uint32_t hours = uptime_seconds / 3600;
        uint32_t minutes = (uptime_seconds % 3600) / 60;
        uint32_t seconds = uptime_seconds % 60;
        
        ESP_LOGI(MAIN_TAG, "Uptime: %02lu:%02lu:%02lu (%lu seconds)", hours, minutes, seconds, uptime_seconds);
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // Log uptime every 5 seconds
    }
}

void app_main(void) {
    ESP_LOGI(MAIN_TAG, "SUB node starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Create mutex for AP list
    ap_list_mutex = xSemaphoreCreateMutex();
    if (ap_list_mutex == NULL) {
        ESP_LOGE(MAIN_TAG, "Failed to create AP list mutex");
        esp_restart();
    }
    
    // Initialize WiFi
    ESP_LOGI(MAIN_TAG, "Initializing WiFi...");
    ret = wifi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize WiFi, restarting...");
        esp_restart();
    }
    
    // Initialize SPI
    ESP_LOGI(MAIN_TAG, "Initializing SPI slave...");
    ret = spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize SPI slave, restarting...");
        esp_restart();
    }
    
    ESP_LOGI(MAIN_TAG, "Starting uptime logging...");
    
    // Create task to log uptime every 5 seconds
    xTaskCreate(uptime_task, "uptime_task", 2048, NULL, 5, NULL);
    
    // Create WiFi scanning task
    xTaskCreate(wifi_scan_task, "wifi_scan_task", 4096, NULL, 6, NULL);
    
    // Create SPI slave task
    xTaskCreate(spi_slave_task, "spi_slave_task", 4096, NULL, 7, NULL);
    
    ESP_LOGI(MAIN_TAG, "SUB node initialized successfully");
}