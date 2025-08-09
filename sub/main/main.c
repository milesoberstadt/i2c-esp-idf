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

// WiFi scanning
#define MAX_AP_RECORDS 500

typedef struct {
  uint64_t first_seen_time;
  int8_t rssi;
  char ssid[33];
  uint8_t bssid[6];
} ap_record_t;

// Global variables
static ap_record_t ap_records[MAX_AP_RECORDS];
static uint32_t ap_count = 0;
static SemaphoreHandle_t ap_list_mutex;
static uint8_t spi_rx_buffer[SPI_BUFFER_SIZE];
static uint8_t spi_tx_buffer[SPI_BUFFER_SIZE];
static bool spi_comm_established = false;

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
    
    // Add new AP if space available
    if (ap_count < MAX_AP_RECORDS) {
      ap_records[ap_count].first_seen_time = esp_timer_get_time();
      ap_records[ap_count].rssi = ap_info->rssi;
      memcpy(ap_records[ap_count].ssid, ap_info->ssid, sizeof(ap_records[ap_count].ssid));
      memcpy(ap_records[ap_count].bssid, ap_info->bssid, 6);
      ap_count++;
      ESP_LOGI(WIFI_TAG, "Added AP %s (RSSI: %d) - Total: %lu", 
               (char*)ap_info->ssid, ap_info->rssi, ap_count);
    } else {
      ESP_LOGW(WIFI_TAG, "AP list full, cannot add %s", (char*)ap_info->ssid);
    }
    
    xSemaphoreGive(ap_list_mutex);
  }
}

void wifi_scan_task(void *pvParameters) {
  ESP_LOGI(WIFI_TAG, "WiFi scanning task waiting for SPI communication to be established...");
  
  // Wait for SPI communication to be established before starting WiFi scanning
  while (!spi_comm_established) {
    vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
  }
  
  ESP_LOGI(WIFI_TAG, "SPI communication established, starting WiFi scanning");
  
  while (1) {
    wifi_scan_config_t scan_config = {
      .ssid = NULL,
      .bssid = NULL,
      .channel = 0,
      .show_hidden = true,
      .scan_type = WIFI_SCAN_TYPE_ACTIVE,
      .scan_time.active.min = 100,
      .scan_time.active.max = 300,
    };
    
    ESP_LOGI(WIFI_TAG, "Starting WiFi scan...");
    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
      ESP_LOGE(WIFI_TAG, "WiFi scan failed: %s", esp_err_to_name(ret));
      vTaskDelay(pdMS_TO_TICKS(5000));
      continue;
    }
    
    uint16_t ap_found = 0;
    esp_wifi_scan_get_ap_num(&ap_found);
    ESP_LOGI(WIFI_TAG, "Found %d access points", ap_found);
    
    if (ap_found > 0) {
      wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_found);
      if (ap_list) {
        esp_wifi_scan_get_ap_records(&ap_found, ap_list);
        
        for (int i = 0; i < ap_found; i++) {
          add_ap_record(&ap_list[i]);
        }
        
        free(ap_list);
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(5000)); // Scan every 5 seconds
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
  
  while (1) {
    memset(spi_rx_buffer, 0, SPI_BUFFER_SIZE);
    memset(spi_tx_buffer, 0, SPI_BUFFER_SIZE);
    
    // Prepare default response (will be overwritten if valid command received)
    uint32_t current_count = 0;
    if (xSemaphoreTake(ap_list_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      current_count = ap_count;
      xSemaphoreGive(ap_list_mutex);
    }
    
    // Pre-fill TX buffer with current WiFi count response
    spi_tx_buffer[0] = CMD_GET_WIFI_COUNT;
    spi_tx_buffer[1] = (current_count >> 24) & 0xFF;
    spi_tx_buffer[2] = (current_count >> 16) & 0xFF;
    spi_tx_buffer[3] = (current_count >> 8) & 0xFF;
    spi_tx_buffer[4] = current_count & 0xFF;
    
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
      if (cmd == CMD_GET_WIFI_COUNT) {
        if (!spi_comm_established) {
          spi_comm_established = true;
          ESP_LOGI(SPI_TAG, "SPI communication established with DOM node!");
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