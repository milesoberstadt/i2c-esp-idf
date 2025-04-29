#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "constants.h"
#include "types.h"
#include "spi_master.h"

static const char *TAG = SPI_TAG;
static spi_device_handle_t spi_device;
static SemaphoreHandle_t spi_mutex;
static uint8_t sub_node_identifier = 0;

// Helper function to log bytes for debugging
static void log_bytes(const char* prefix, uint8_t* data, size_t len) {
    char buffer[100];
    int offset = 0;
    
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s: [", prefix);
    for (size_t i = 0; i < len && i < 16; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%02X ", data[i]);
    }
    if (len > 16) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "...");
    } else {
        // Remove trailing space
        if (len > 0) {
            buffer[offset-1] = ']';
        } else {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "]\n");
        }
    }
    
    ESP_LOGI(TAG, "%s", buffer);
}

bool spi_master_init(void) {
    ESP_LOGI(TAG, "Initializing SPI master");
    
    // Check if device is already initialized
    if (spi_device) {
        ESP_LOGW(TAG, "SPI device already initialized, deinitializing first");
        spi_master_deinit();
    }
    
    // Create mutex for SPI bus access
    if (spi_mutex == NULL) {
        spi_mutex = xSemaphoreCreateMutex();
        if (spi_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create SPI mutex");
            return false;
        }
    }
    
    // Initialize SPI bus configuration
    spi_bus_config_t bus_config = {
        .mosi_io_num = SPI_MOSI_PIN,
        .miso_io_num = SPI_MISO_PIN,
        .sclk_io_num = SPI_SCLK_PIN,
        .quadwp_io_num = -1,  // Not used
        .quadhd_io_num = -1,  // Not used
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE * 2,  // Double size for safety
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .intr_flags = 0
    };
    
    // Initialize SPI device configuration
    spi_device_interface_config_t device_config = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,  // SPI mode 0 (CPOL=0, CPHA=0)
        .duty_cycle_pos = 128,  // 50% duty cycle
        .cs_ena_pretrans = 1,   // CS setup time
        .cs_ena_posttrans = 1,  // CS hold time
        .clock_speed_hz = SPI_CLOCK_SPEED_HZ,
        .spics_io_num = SPI_CS_PIN,
        .flags = 0,
        .queue_size = SPI_QUEUE_SIZE,
        .pre_cb = NULL,
        .post_cb = NULL
    };
    
    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(SPI_HOST, &bus_config, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "SPI bus initialized successfully");
    
    // Add device to the SPI bus
    ret = spi_bus_add_device(SPI_HOST, &device_config, &spi_device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device to SPI bus: %s", esp_err_to_name(ret));
        spi_bus_free(SPI_HOST);
        return false;
    }
    
    ESP_LOGI(TAG, "SPI device added successfully");
    return true;
}

void spi_master_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing SPI master");
    
    if (spi_device) {
        spi_bus_remove_device(spi_device);
        spi_device = NULL;
    }
    
    spi_bus_free(SPI_HOST);
    ESP_LOGI(TAG, "SPI master deinitialization successful");
}

bool spi_exchange_data(uint8_t *tx_data, uint8_t *rx_data, size_t len) {
    if (!spi_device) {
        ESP_LOGE(TAG, "SPI device not initialized");
        return false;
    }
    
    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take SPI mutex");
        return false;
    }
    
    // Pre-clear receive buffer to avoid reading garbage
    memset(rx_data, 0, len);
    
    // Single transaction: Send command and receive response directly
    spi_transaction_t transaction = {
        .length = len * 8,  // Length in bits
        .tx_buffer = tx_data,
        .rx_buffer = rx_data
    };

    ESP_LOGI(TAG, "SENDING MESSAGE:");
    for (int i = 0; i < 32; i += 8) {
        ESP_LOGI(TAG, "  [%02X %02X %02X %02X %02X %02X %02X %02X]",
                 tx_data[i], tx_data[i + 1], tx_data[i + 2], tx_data[i + 3],
                 tx_data[i + 4], tx_data[i + 5], tx_data[i + 6], tx_data[i + 7]);
    }

    // Use standard transmit - interrupt-based for ESP-IDF 5.4
    esp_err_t ret = spi_device_transmit(spi_device, &transaction);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transaction failed: %s", esp_err_to_name(ret));
        xSemaphoreGive(spi_mutex);
        return false;
    }

    // Log received data
    ESP_LOGI(TAG, "RECEIVED RESPONSE:");
    for (int i = 0; i < 32; i += 8) {
        ESP_LOGI(TAG, "  [%02X %02X %02X %02X %02X %02X %02X %02X]",
                 rx_data[i], rx_data[i + 1], rx_data[i + 2], rx_data[i + 3],
                 rx_data[i + 4], rx_data[i + 5], rx_data[i + 6], rx_data[i + 7]);
    }

    // Add a small delay after transaction to let slave prepare for next one
    vTaskDelay(pdMS_TO_TICKS(10));
    
    xSemaphoreGive(spi_mutex);
    return true;
}

bool spi_check_slave(void) {
    uint8_t tx_buffer[SPI_MAX_TRANSFER_SIZE] = {0};
    uint8_t rx_buffer[SPI_MAX_TRANSFER_SIZE] = {0};
    
    ESP_LOGI(TAG, "Checking SPI slave connectivity");
    
    bool connected = false;
    const int max_attempts = 10;  // Increase attempts
    
    // Send a dummy message first to wake up the slave
    memset(tx_buffer, 0, SPI_MAX_TRANSFER_SIZE);
    spi_exchange_data(tx_buffer, rx_buffer, SPI_MAX_TRANSFER_SIZE);
    
    // Wait for slave to process this initial message
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Now prepare the real identifier request
    tx_buffer[0] = msg_req_identifier;
    tx_buffer[1] = 0;  // Device index 0
    tx_buffer[2] = 0;  // No data
    
    // Fill remaining bytes with 0x00 (as padding)
    for (int i = 3; i < SPI_MAX_TRANSFER_SIZE; i++) {
        tx_buffer[i] = 0x00;
    }
    
    for (int attempt = 1; attempt <= max_attempts; attempt++) {
        ESP_LOGI(TAG, "Connection attempt %d/%d", attempt, max_attempts);
        
        // Clear rx buffer before each attempt
        memset(rx_buffer, 0, SPI_MAX_TRANSFER_SIZE);
        
        if (spi_exchange_data(tx_buffer, rx_buffer, SPI_MAX_TRANSFER_SIZE)) {
            // Log first few bytes of response
            ESP_LOGI(TAG, "Response bytes: [%02X %02X %02X %02X]", 
                    rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3]);
            
            // Look for identifier response message
            if (rx_buffer[0] == msg_res_identifier) {
                sub_node_identifier = rx_buffer[3];  // Store the identifier
                ESP_LOGI(TAG, "SPI slave is responsive, bytes:[%02X %02X %02X %02X], using identifier: 0x%02X",
                        rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3], sub_node_identifier);
                connected = true;
                break;
            } else {
                ESP_LOGW(TAG, "Invalid response from SPI slave: 0x%02X (decimal %d)", rx_buffer[0], rx_buffer[0]);
            }
        }
        
        // Wait before retrying - progressively longer waits
        if (attempt < max_attempts) {
            ESP_LOGI(TAG, "Sending another ping message");
            vTaskDelay(pdMS_TO_TICKS(100 + (attempt * 20)));
        }
    }
    
    if (!connected) {
        ESP_LOGE(TAG, "Failed to get valid response from SPI slave after %d attempts", max_attempts);
    }
    
    return connected;
}

bool spi_send_message(message_t msg_type, uint8_t device_index) {
    uint8_t tx_buffer[SPI_MAX_TRANSFER_SIZE] = {0};
    uint8_t rx_buffer[SPI_MAX_TRANSFER_SIZE] = {0};
    
    // Prepare message
    tx_buffer[0] = msg_type;
    tx_buffer[1] = device_index;
    tx_buffer[2] = 0;  // No data
    
    // Fill remaining bytes with 0x00 (as padding)
    for (int i = 3; i < SPI_MAX_TRANSFER_SIZE; i++) {
        tx_buffer[i] = 0x00;
    }
    
    if (!spi_exchange_data(tx_buffer, rx_buffer, SPI_MAX_TRANSFER_SIZE)) {
        ESP_LOGE(TAG, "Failed to send message type 0x%02X", msg_type);
        return false;
    }
    
    // Check response for error
    if (rx_buffer[0] == msg_err) {
        ESP_LOGW(TAG, "Received error response for message type 0x%02X", msg_type);
        return false;
    }
    
    return true;
}

bool spi_send_message_data(message_t msg_type, uint8_t device_index, const uint8_t *data, size_t data_len) {
    if (data_len > SPI_MAX_TRANSFER_SIZE - 3) {
        ESP_LOGE(TAG, "Data length exceeds maximum allowed size");
        return false;
    }
    
    uint8_t tx_buffer[SPI_MAX_TRANSFER_SIZE] = {0};
    uint8_t rx_buffer[SPI_MAX_TRANSFER_SIZE] = {0};
    
    // Prepare message
    tx_buffer[0] = msg_type;
    tx_buffer[1] = device_index;
    tx_buffer[2] = data_len;
    
    // Copy data payload
    if (data && data_len > 0) {
        memcpy(&tx_buffer[3], data, data_len);
    }
    
    // Fill remaining bytes with 0x00 (as padding)
    for (int i = 3 + data_len; i < SPI_MAX_TRANSFER_SIZE; i++) {
        tx_buffer[i] = 0x00;
    }
    
    if (!spi_exchange_data(tx_buffer, rx_buffer, SPI_MAX_TRANSFER_SIZE)) {
        ESP_LOGE(TAG, "Failed to send message type 0x%02X with data", msg_type);
        return false;
    }
    
    // Check response for error
    if (rx_buffer[0] == msg_err) {
        ESP_LOGW(TAG, "Received error response for message type 0x%02X with data", msg_type);
        return false;
    }
    
    return true;
}

bool spi_request_data(uint8_t device_index, uint8_t *rx_data, size_t *rx_data_len) {
    if (!rx_data || !rx_data_len || *rx_data_len == 0) {
        ESP_LOGE(TAG, "Invalid receive buffer or length");
        return false;
    }
    
    uint8_t tx_buffer[SPI_MAX_TRANSFER_SIZE] = {0};
    uint8_t rx_buffer[SPI_MAX_TRANSFER_SIZE] = {0};
    
    // Prepare data request message
    tx_buffer[0] = msg_req_data;
    tx_buffer[1] = device_index;
    tx_buffer[2] = 0;  // No data in request
    
    // Fill remaining bytes with 0x00 (as padding)
    for (int i = 3; i < SPI_MAX_TRANSFER_SIZE; i++) {
        tx_buffer[i] = 0x00;
    }
    
    // Send request and get response directly
    if (!spi_exchange_data(tx_buffer, rx_buffer, SPI_MAX_TRANSFER_SIZE)) {
        ESP_LOGE(TAG, "Failed to send data request");
        return false;
    }
    
    // Check if response is a data response
    if (rx_buffer[0] != msg_res_data) {
        ESP_LOGW(TAG, "Unexpected response type: 0x%02X (expected 0x%02X)", 
                rx_buffer[0], msg_res_data);
        return false;
    }
    
    // Get data length from response
    uint8_t data_len = rx_buffer[2];
    
    // Check if data length is valid
    if (data_len > SPI_MAX_TRANSFER_SIZE - 3 || data_len > *rx_data_len) {
        ESP_LOGW(TAG, "Data length in response (%d) exceeds buffer size", data_len);
        *rx_data_len = data_len;  // Return actual data length
        return false;
    }
    
    // Copy data to output buffer
    memcpy(rx_data, &rx_buffer[3], data_len);
    *rx_data_len = data_len;
    
    return true;
}

bool spi_set_wifi_channel(uint8_t channel) {
    ESP_LOGI(TAG, "Setting WiFi channel to %d", channel);
    
    uint8_t tx_buffer[SPI_MAX_TRANSFER_SIZE] = {0};
    uint8_t rx_buffer[SPI_MAX_TRANSFER_SIZE] = {0};
    
    // Prepare channel setting message
    tx_buffer[0] = msg_set_wifi_channel;
    tx_buffer[1] = 0;  // Device index
    tx_buffer[2] = 1;  // Data length (1 byte for channel)
    tx_buffer[3] = channel;
    
    // Fill remaining bytes with 0x00 (as padding)
    for (int i = 4; i < SPI_MAX_TRANSFER_SIZE; i++) {
        tx_buffer[i] = 0x00;
    }
    
    if (!spi_exchange_data(tx_buffer, rx_buffer, SPI_MAX_TRANSFER_SIZE)) {
        ESP_LOGE(TAG, "Failed to send WiFi channel setting");
        return false;
    }
    
    // Check response for error
    if (rx_buffer[0] == msg_err) {
        ESP_LOGW(TAG, "Received error response for WiFi channel setting");
        return false;
    }
    
    return true;
}
