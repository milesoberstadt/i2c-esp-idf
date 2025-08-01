#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define MAIN_TAG "DOM_MAIN"
#define SPI_TAG "DOM_SPI"

// SPI pin definitions based on README.md
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

// SPI configuration
#define SPI_HOST_ID  SPI2_HOST
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define SPI_MAX_TRANSFER_SIZE 64

// Communication protocol
#define CMD_GET_WIFI_COUNT 0x01
#define RESPONSE_TIMEOUT_MS 1000
#define MAX_RETRIES 3

static spi_device_handle_t spi_handle;

esp_err_t spi_init(void) {
  spi_bus_config_t buscfg = {
    .miso_io_num = PIN_NUM_MISO,
    .mosi_io_num = PIN_NUM_MOSI,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
  };

  esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &buscfg, SPI_DMA_CHAN);
  if (ret != ESP_OK) {
    ESP_LOGE(SPI_TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
    return ret;
  }

  spi_device_interface_config_t devcfg = {
    .clock_speed_hz = 1000000,  // 1 MHz
    .mode = 0,
    .spics_io_num = PIN_NUM_CS,
    .queue_size = 7,
  };

  ret = spi_bus_add_device(SPI_HOST_ID, &devcfg, &spi_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(SPI_TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(SPI_TAG, "SPI initialized successfully");
  return ESP_OK;
}

esp_err_t spi_send_command_with_retry(uint8_t cmd, uint32_t *response) {
  for (int retry = 0; retry < MAX_RETRIES; retry++) {
    uint8_t tx_data[8] = {cmd, 0, 0, 0, 0, 0, 0, 0};
    uint8_t rx_data[8] = {0};

    spi_transaction_t trans = {
      .length = 8 * 8,  // 8 bytes * 8 bits
      .tx_buffer = tx_data,
      .rx_buffer = rx_data,
    };

    ESP_LOGI(SPI_TAG, "Sending command 0x%02X (attempt %d/%d)", cmd, retry + 1, MAX_RETRIES);

    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK) {
      ESP_LOGE(SPI_TAG, "SPI transmit failed: %s", esp_err_to_name(ret));
      if (retry < MAX_RETRIES - 1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
      }
      return ret;
    }

    // Log SPI traffic
    ESP_LOGI(SPI_TAG, "TX: %02X %02X %02X %02X %02X %02X %02X %02X", 
             tx_data[0], tx_data[1], tx_data[2], tx_data[3],
             tx_data[4], tx_data[5], tx_data[6], tx_data[7]);
    ESP_LOGI(SPI_TAG, "RX: %02X %02X %02X %02X %02X %02X %02X %02X", 
             rx_data[0], rx_data[1], rx_data[2], rx_data[3],
             rx_data[4], rx_data[5], rx_data[6], rx_data[7]);

    // Extract response (assuming 4-byte response starting at index 1)
    *response = (rx_data[1] << 24) | (rx_data[2] << 16) | (rx_data[3] << 8) | rx_data[4];
    
    // Check if we got a valid response (non-zero or expected pattern)
    if (rx_data[0] == cmd || *response != 0) {
      ESP_LOGI(SPI_TAG, "Command successful, response: %lu", *response);
      return ESP_OK;
    }

    ESP_LOGW(SPI_TAG, "No response from SUB node, retrying...");
    if (retry < MAX_RETRIES - 1) {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  ESP_LOGE(SPI_TAG, "Failed to get response after %d retries", MAX_RETRIES);
  return ESP_FAIL;
}

void spi_communication_task(void *pvParameters) {
  ESP_LOGI(SPI_TAG, "Waiting 5 seconds for SUB node to boot...");
  vTaskDelay(pdMS_TO_TICKS(5000));

  while (1) {
    uint32_t wifi_count = 0;
    esp_err_t ret = spi_send_command_with_retry(CMD_GET_WIFI_COUNT, &wifi_count);
    
    if (ret == ESP_OK) {
      ESP_LOGI(MAIN_TAG, "WiFi AP count from SUB node: %lu", wifi_count);
    } else {
      ESP_LOGE(MAIN_TAG, "Failed to get WiFi count from SUB node");
    }

    vTaskDelay(pdMS_TO_TICKS(10000)); // Wait 10 seconds before next request
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
    ESP_LOGI(MAIN_TAG, "DOM node starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize SPI
    ESP_LOGI(MAIN_TAG, "Initializing SPI...");
    ret = spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize SPI, restarting...");
        esp_restart();
    }
    
    ESP_LOGI(MAIN_TAG, "Starting uptime logging...");
    
    // Create task to log uptime every 5 seconds
    xTaskCreate(uptime_task, "uptime_task", 2048, NULL, 5, NULL);
    
    // Create task for SPI communication
    xTaskCreate(spi_communication_task, "spi_comm_task", 4096, NULL, 6, NULL);
    
    ESP_LOGI(MAIN_TAG, "DOM node initialized successfully");
}