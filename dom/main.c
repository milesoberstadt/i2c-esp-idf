#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define MAIN_TAG "DOM_MAIN"
#define SPI_TAG "DOM_SPI"

// SPI pin definitions for Raspberry Pi Pico
#define PIN_MISO 16    // GP16 (SPI0 RX)
#define PIN_MOSI 19    // GP19 (SPI0 TX)
#define PIN_CLK  18    // GP18 (SPI0 SCK)

// CS pins for SUB nodes (using GPIO pins)
static const uint cs_pins[] = {17, 20}; // GP17, GP20
#define NUM_SUB_NODES (sizeof(cs_pins) / sizeof(cs_pins[0]))

// SPI configuration
#define SPI_PORT spi0
#define SPI_BAUDRATE 1000000  // 1 MHz

// Communication protocol
#define CMD_GET_WIFI_COUNT 0x01
#define RESPONSE_TIMEOUT_MS 1000

// Logging macros (simplified for Pico)
#define LOG_INFO(tag, format, ...) printf("[%s] " format "\n", tag, ##__VA_ARGS__)
#define LOG_WARN(tag, format, ...) printf("[%s] WARN: " format "\n", tag, ##__VA_ARGS__)
#define LOG_ERROR(tag, format, ...) printf("[%s] ERROR: " format "\n", tag, ##__VA_ARGS__)

bool gpio_init_cs_pins(void) {
  for (int i = 0; i < NUM_SUB_NODES; i++) {
    gpio_init(cs_pins[i]);
    gpio_set_dir(cs_pins[i], GPIO_OUT);
    gpio_put(cs_pins[i], 1); // Set CS pins high (inactive)
  }
  LOG_INFO(SPI_TAG, "Initialized %d CS pins", NUM_SUB_NODES);
  return true;
}

bool spi_init_dom(void) {
  // Initialize CS pins as GPIO outputs
  if (!gpio_init_cs_pins()) {
    return false;
  }

  // Initialize SPI at 1 MHz
  spi_init(SPI_PORT, SPI_BAUDRATE);

  // Set SPI pins
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PIN_CLK, GPIO_FUNC_SPI);

  LOG_INFO(SPI_TAG, "SPI initialized successfully with manual CS control");
  return true;
}

bool spi_send_command_to_node(uint cs_pin, uint8_t cmd, uint32_t *response) {
  // Clear buffers
  uint8_t tx_data[8] = {0};
  uint8_t rx_data[8] = {0};

  // Prepare command
  tx_data[0] = cmd;

  LOG_INFO(SPI_TAG, "Sending command 0x%02X to CS pin %d", cmd, cs_pin);

  // Manual CS control - select the device
  gpio_put(cs_pin, 0);  // Active low CS
  sleep_ms(10); // Small delay for CS setup

  // Perform SPI transaction
  int bytes_transferred = spi_write_read_blocking(SPI_PORT, tx_data, rx_data, 8);

  // Deselect the device
  gpio_put(cs_pin, 1);  // Inactive high CS

  if (bytes_transferred != 8) {
    LOG_ERROR(SPI_TAG, "SPI transmit failed: only %d bytes transferred", bytes_transferred);
    return false;
  }

  // Log SPI traffic
  LOG_INFO(SPI_TAG, "TX: %02X %02X %02X %02X %02X %02X %02X %02X",
           tx_data[0], tx_data[1], tx_data[2], tx_data[3],
           tx_data[4], tx_data[5], tx_data[6], tx_data[7]);
  LOG_INFO(SPI_TAG, "RX: %02X %02X %02X %02X %02X %02X %02X %02X",
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
    LOG_WARN(SPI_TAG, "Floating bus detected (all 0xFF) on CS pin %d - no device connected", cs_pin);
    return false;
  }

  if (all_00) {
    LOG_WARN(SPI_TAG, "No response (all 0x00) from CS pin %d - device not ready", cs_pin);
    return false;
  }

  // Check if first byte echoes our command (valid SUB node response)
  if (rx_data[0] != cmd) {
    LOG_WARN(SPI_TAG, "Invalid response header 0x%02X (expected 0x%02X) from CS pin %d", rx_data[0], cmd, cs_pin);
    return false;
  }

  // Extract response (4-byte response starting at index 1)
  *response = (rx_data[1] << 24) | (rx_data[2] << 16) | (rx_data[3] << 8) | rx_data[4];

  // Real SUB nodes ALWAYS send zeros in bytes 5-7 (unused buffer space)
  if (rx_data[5] != 0 || rx_data[6] != 0 || rx_data[7] != 0) {
    LOG_WARN(SPI_TAG, "Invalid response format from CS pin %d (bytes 5-7: %02X %02X %02X) - likely no device connected",
             cs_pin, rx_data[5], rx_data[6], rx_data[7]);
    return false;
  }

  LOG_INFO(SPI_TAG, "Valid response from CS pin %d: %lu", cs_pin, *response);
  return true;
}

void poll_sub_nodes(void) {
  LOG_INFO(MAIN_TAG, "Polling %d SUB nodes...", NUM_SUB_NODES);

  for (int i = 0; i < NUM_SUB_NODES; i++) {
    uint cs_pin = cs_pins[i];
    LOG_INFO(MAIN_TAG, "Querying SUB node %d (CS pin %d)", i + 1, cs_pin);

    uint32_t wifi_count = 0;
    bool success = spi_send_command_to_node(cs_pin, CMD_GET_WIFI_COUNT, &wifi_count);

    if (success) {
      LOG_INFO(MAIN_TAG, "SUB node %d (CS pin %d): WiFi AP count = %lu", i + 1, cs_pin, wifi_count);
    } else {
      LOG_INFO(MAIN_TAG, "SUB node %d (CS pin %d): No response (device not connected)", i + 1, cs_pin);
    }

    // Small delay between nodes to avoid bus conflicts
    sleep_ms(50);
  }

  LOG_INFO(MAIN_TAG, "Completed polling all SUB nodes");
}

void log_uptime(void) {
  uint32_t uptime_ms = to_ms_since_boot(get_absolute_time());
  uint32_t uptime_seconds = uptime_ms / 1000;
  uint32_t hours = uptime_seconds / 3600;
  uint32_t minutes = (uptime_seconds % 3600) / 60;
  uint32_t seconds = uptime_seconds % 60;

  LOG_INFO(MAIN_TAG, "Uptime: %02lu:%02lu:%02lu (%lu seconds)", hours, minutes, seconds, uptime_seconds);
}

int main() {
  stdio_init_all();

  LOG_INFO(MAIN_TAG, "DOM node starting...");

  // Wait for SUB nodes to boot
  LOG_INFO(MAIN_TAG, "Waiting 5 seconds for SUB nodes to boot...");
  sleep_ms(5000);

  // Initialize SPI
  LOG_INFO(SPI_TAG, "Initializing SPI...");
  if (!spi_init_dom()) {
    LOG_ERROR(SPI_TAG, "Failed to initialize SPI");
    return 1;
  }

  LOG_INFO(MAIN_TAG, "DOM node initialized successfully");

  // Timing variables
  uint32_t last_poll_time = 0;
  uint32_t last_uptime_log_time = 0;
  const uint32_t POLL_INTERVAL_MS = 10000;  // Poll every 10 seconds
  const uint32_t UPTIME_LOG_INTERVAL_MS = 5000;  // Log uptime every 5 seconds

  // Main loop
  while (1) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    // Poll SUB nodes periodically
    if (current_time - last_poll_time >= POLL_INTERVAL_MS) {
      poll_sub_nodes();
      last_poll_time = current_time;
    }

    // Log uptime periodically
    if (current_time - last_uptime_log_time >= UPTIME_LOG_INTERVAL_MS) {
      log_uptime();
      last_uptime_log_time = current_time;
    }

    // Small sleep to avoid busy-waiting
    sleep_ms(100);
  }

  return 0;
}