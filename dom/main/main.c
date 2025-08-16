#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
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

// CS pins for SUB nodes (up to 11 nodes from README.md)
static const int cs_pins[] = {5, 4};
// static const int cs_pins[] = {5, 4, 21, 22, 32, 33, 25, 26, 27, 2, 16};
#define NUM_SUB_NODES (sizeof(cs_pins) / sizeof(cs_pins[0]))

// SPI configuration
#define SPI_HOST_ID  SPI2_HOST
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define SPI_MAX_TRANSFER_SIZE 64

// Communication protocol
#define CMD_GET_WIFI_COUNT 0x01
#define RESPONSE_TIMEOUT_MS 1000

static spi_device_handle_t spi_handle = NULL;

esp_err_t gpio_init_cs_pins(void) {
  for (int i = 0; i < NUM_SUB_NODES; i++) {
    gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = (1ULL << cs_pins[i]),
      .pull_down_en = 0,
      .pull_up_en = 0,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
      ESP_LOGE(SPI_TAG, "Failed to configure CS pin %d: %s", cs_pins[i], esp_err_to_name(ret));
      return ret;
    }
    // Set CS pins high (inactive)
    gpio_set_level(cs_pins[i], 1);
  }
  ESP_LOGI(SPI_TAG, "Initialized %d CS pins", NUM_SUB_NODES);
  return ESP_OK;
}

esp_err_t spi_init(void) {
  // Initialize CS pins as GPIO outputs
  esp_err_t ret = gpio_init_cs_pins();
  if (ret != ESP_OK) {
    return ret;
  }

  spi_bus_config_t buscfg = {
    .miso_io_num = PIN_NUM_MISO,
    .mosi_io_num = PIN_NUM_MOSI,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
  };

  ret = spi_bus_initialize(SPI_HOST_ID, &buscfg, SPI_DMA_CHAN);
  if (ret != ESP_OK) {
    ESP_LOGE(SPI_TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
    return ret;
  }

  // Add single SPI device with no CS pin (we'll control CS manually)
  spi_device_interface_config_t devcfg = {
    .clock_speed_hz = 1000000,  // 1 MHz
    .mode = 0,
    .spics_io_num = -1,  // No automatic CS control
    .queue_size = 7,
  };

  ret = spi_bus_add_device(SPI_HOST_ID, &devcfg, &spi_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(SPI_TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(SPI_TAG, "SPI initialized successfully with manual CS control");
  return ESP_OK;
}

esp_err_t spi_send_command_to_node(int cs_pin, uint8_t cmd, uint32_t *response) {
  // Clear buffers
  uint8_t tx_data[8] = {0};
  uint8_t rx_data[8] = {0};
  
  // Prepare command
  tx_data[0] = cmd;

  spi_transaction_t trans = {
    .length = 8 * 8,  // 8 bytes * 8 bits
    .tx_buffer = tx_data,
    .rx_buffer = rx_data,
  };

  ESP_LOGI(SPI_TAG, "Sending command 0x%02X to CS pin %d", cmd, cs_pin);

  // Manual CS control
  for (int i = 0; i < NUM_SUB_NODES; i++) {
    gpio_set_level(cs_pins[i], 1);  // Deselect all devices
  }
  esp_rom_delay_us(2);  // settle time
  gpio_set_level(cs_pin, 0);  // Active low CS
  esp_rom_delay_us(5);  // Additional delay for device readiness

  esp_err_t ret = spi_device_transmit(spi_handle, &trans);
  
  // Deselect the device
  gpio_set_level(cs_pin, 1);  // Inactive high CS
  esp_rom_delay_us(2);  // settle time

  if (ret != ESP_OK) {
    ESP_LOGE(SPI_TAG, "SPI transmit failed: %s", esp_err_to_name(ret));
    return ret;
  }

  // Log SPI traffic
  ESP_LOGI(SPI_TAG, "TX: %02X %02X %02X %02X %02X %02X %02X %02X", 
           tx_data[0], tx_data[1], tx_data[2], tx_data[3],
           tx_data[4], tx_data[5], tx_data[6], tx_data[7]);
  ESP_LOGI(SPI_TAG, "RX: %02X %02X %02X %02X %02X %02X %02X %02X", 
           rx_data[0], rx_data[1], rx_data[2], rx_data[3],
           rx_data[4], rx_data[5], rx_data[6], rx_data[7]);

  // Check for floating bus conditions (all 0xFF or all 0x00)
  bool all_ff = true;
  bool all_00 = true;
  for (int i = 0; i < 8; i++) {
    if (rx_data[i] != 0xFF) all_ff = false;
    if (rx_data[i] != 0x00) all_00 = false;
  }
  
  if (all_ff) {
    ESP_LOGW(SPI_TAG, "Floating bus detected (all 0xFF) on CS pin %d - no device connected", cs_pin);
    return ESP_FAIL;
  }
  
  if (all_00) {
    ESP_LOGW(SPI_TAG, "No response (all 0x00) from CS pin %d - device not ready", cs_pin);
    return ESP_FAIL;
  }
  
  // Check if first byte echoes our command (valid SUB node response)
  if (rx_data[0] != cmd) {
    ESP_LOGW(SPI_TAG, "Invalid response header 0x%02X (expected 0x%02X) from CS pin %d", rx_data[0], cmd, cs_pin);
    return ESP_FAIL;
  }
  
  // Extract response (4-byte response starting at index 1)
  *response = (rx_data[1] << 24) | (rx_data[2] << 16) | (rx_data[3] << 8) | rx_data[4];
  
  // Real SUB nodes ALWAYS send zeros in bytes 5-7 (unused buffer space)
  // If these bytes contain garbage data, it's likely an unconnected device with floating pins
  if (rx_data[5] != 0 || rx_data[6] != 0 || rx_data[7] != 0) {
    ESP_LOGW(SPI_TAG, "Invalid response format from CS pin %d (bytes 5-7: %02X %02X %02X) - likely no device connected", 
             cs_pin, rx_data[5], rx_data[6], rx_data[7]);
    return ESP_FAIL;
  }
  
  ESP_LOGI(SPI_TAG, "Valid response from CS pin %d: %lu", cs_pin, *response);
  return ESP_OK;
}

void spi_communication_task(void *pvParameters) {
  ESP_LOGI(SPI_TAG, "Waiting 5 seconds for SUB nodes to boot...");
  vTaskDelay(pdMS_TO_TICKS(5000));

  // Initialize SPI after SUB nodes have had time to boot
  ESP_LOGI(SPI_TAG, "Initializing SPI after SUB boot delay...");
  esp_err_t ret = spi_init();
  if (ret != ESP_OK) {
    ESP_LOGE(SPI_TAG, "Failed to initialize SPI, restarting...");
    esp_restart();
  }

  while (1) {
    ESP_LOGI(MAIN_TAG, "Polling %d SUB nodes...", NUM_SUB_NODES);
    
    for (int i = 0; i < NUM_SUB_NODES; i++) {
      int cs_pin = cs_pins[i];
      ESP_LOGI(MAIN_TAG, "Querying SUB node %d (CS pin %d)", i + 1, cs_pin);
      
      uint32_t wifi_count = 0;
      esp_err_t ret = spi_send_command_to_node(cs_pin, CMD_GET_WIFI_COUNT, &wifi_count);
      
      if (ret == ESP_OK) {
        ESP_LOGI(MAIN_TAG, "SUB node %d (CS pin %d): WiFi AP count = %lu", i + 1, cs_pin, wifi_count);
      } else {
        ESP_LOGI(MAIN_TAG, "SUB node %d (CS pin %d): No response (device not connected)", i + 1, cs_pin);
      }
      
      // Small delay between nodes to avoid bus conflicts
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    ESP_LOGI(MAIN_TAG, "Completed polling all SUB nodes");
    vTaskDelay(pdMS_TO_TICKS(5000)); // Wait 5 seconds before next polling cycle
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
    
    ESP_LOGI(MAIN_TAG, "Starting uptime logging...");
    
    // Create task for SPI communication
    xTaskCreate(spi_communication_task, "spi_comm_task", 4096, NULL, 6, NULL);
    
    ESP_LOGI(MAIN_TAG, "DOM node initialized successfully");
}