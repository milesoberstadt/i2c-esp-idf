#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/spi_slave.h"
#include "driver/gpio.h"

#include "constants.h"
#include "types.h"
#include "spi_slave.h"
#include "wifi_sniffer.h"

static const char *TAG = SPI_SLAVE_TAG;
static QueueHandle_t spi_event_queue = NULL;
static SemaphoreHandle_t spi_mutex = NULL;
static TaskHandle_t spi_task_handle = NULL;
static uint8_t node_identifier = 0x42;  // Default identifier
static uint8_t wifi_channel = DEFAULT_WIFI_CHANNEL;

// Buffer for SPI transactions
DRAM_ATTR static uint8_t spi_rx_buffer[SPI_MAX_TRANSFER_SIZE];
DRAM_ATTR static uint8_t spi_tx_buffer[SPI_MAX_TRANSFER_SIZE];

// Prepare transaction for the next transfer
static void prepare_next_transaction(void) {
    // Clear buffers
    memset(spi_rx_buffer, 0, SPI_MAX_TRANSFER_SIZE);
    
    // Default response is identifier response
    spi_tx_buffer[0] = msg_res_identifier;
    spi_tx_buffer[1] = 0;  // Reserved
    spi_tx_buffer[2] = 1;  // Data length (identifier)
    spi_tx_buffer[3] = node_identifier;
    
    // Fill rest with 0xFF (padding)
    for (int i = 4; i < SPI_MAX_TRANSFER_SIZE; i++) {
        spi_tx_buffer[i] = 0xFF;
    }
}

// Handle received SPI message
static void handle_spi_message(const uint8_t *rx_data, uint8_t *tx_response) {
    // Handle msg_req_identifier directly even if it's not perfectly formed
    // This is common for first message after initialization
    if (rx_data[0] == msg_req_identifier || 
       (rx_data[0] == 0 && rx_data[1] == 0 && rx_data[2] == 0)) { // blank message or identifier request
        
        // Prepare with identifier response
        tx_response[0] = msg_res_identifier;  // Send identifier
        tx_response[1] = 0;                   // Device index
        tx_response[2] = 1;                   // Data length
        tx_response[3] = node_identifier;     // Identifier value
        
        // Fill rest with 0xFF (padding)
        for (int i = 4; i < SPI_MAX_TRANSFER_SIZE; i++) {
            tx_response[i] = 0xFF;
        }
        
        if (rx_data[0] == 0) {
            ESP_LOGI(TAG, "Received blank message, responding with identifier");
        } else {
            ESP_LOGI(TAG, "Received identifier request");
        }
        return;
    }
    
    message_t msg_type = (message_t)rx_data[0];
    uint8_t device_index = rx_data[1];
    uint8_t data_len = rx_data[2];

    ESP_LOGI(TAG, "INCOMING MESSAGE:");
    for (int i = 0; i < 32; i += 8){
        ESP_LOGI(TAG, "  [%02X %02X %02X %02X %02X %02X %02X %02X]",
                 rx_data[i], rx_data[i + 1], rx_data[i + 2], rx_data[i + 3],
                 rx_data[i + 4], rx_data[i + 5], rx_data[i + 6], rx_data[i + 7]);
    }
    
    // Default response is error (if we can't process the message)
    tx_response[0] = msg_err;
    tx_response[1] = 0;
    tx_response[2] = 0;
    
    // Fill rest with 0xFF (padding)
    for (int i = 3; i < SPI_MAX_TRANSFER_SIZE; i++) {
        tx_response[i] = 0xFF;
    }
    
    // Process message based on type
    switch (msg_type) {
        case msg_init_start:
            ESP_LOGI(TAG, "Received initialization start");
            tx_response[0] = msg_init_start;  // Echo back the command
            break;
            
        case msg_init_end:
            ESP_LOGI(TAG, "Received initialization end");
            tx_response[0] = msg_init_end;  // Echo back the command
            break;
            
        case msg_data:
            ESP_LOGI(TAG, "Received data message with %d bytes", data_len);
            tx_response[0] = msg_data;  // Echo back the command
            break;
            
        case msg_req_data:
            ESP_LOGI(TAG, "Received data request");
            
            // Prepare a data response
            tx_response[0] = msg_res_data;
            tx_response[1] = device_index;
            
            // For now just send a simple test payload
            tx_response[2] = 4;  // 4 bytes of data
            tx_response[3] = 0xAA;
            tx_response[4] = 0xBB;
            tx_response[5] = 0xCC;
            tx_response[6] = 0xDD;
            break;
            
        case msg_req_identifier:
            ESP_LOGI(TAG, "Received identifier request");
            
            // Send identifier response
            tx_response[0] = msg_res_identifier;
            tx_response[1] = device_index;
            tx_response[2] = 1;  // 1 byte of data (identifier)
            tx_response[3] = node_identifier;
            break;
            
        case msg_set_wifi_channel:
            if (data_len >= 1) {
                uint8_t new_channel = rx_data[3];
                ESP_LOGI(TAG, "Received request to set WiFi channel to %d", new_channel);
                
                if (new_channel >= 1 && new_channel <= 14) {
                    wifi_channel = new_channel;
                    wifi_sniffer_set_channel(wifi_channel);
                    
                    // Send confirmation response
                    tx_response[0] = msg_set_wifi_channel;
                    tx_response[1] = device_index;
                    tx_response[2] = 1;  // 1 byte of data (channel)
                    tx_response[3] = wifi_channel;
                } else {
                    ESP_LOGE(TAG, "Invalid WiFi channel: %d (must be 1-14)", new_channel);
                    // Keep error response
                }
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown message type: 0x%02X", msg_type);
            // Keep error response
            break;
    }
}

// SPI transaction post-processing callback 
static void IRAM_ATTR spi_post_trans_callback(spi_slave_transaction_t *trans) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(spi_event_queue, &trans, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// Task that processes SPI transactions
static void spi_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting SPI receive task");
    
    spi_slave_transaction_t trans;
    
    // Prepare initial transaction buffer
    prepare_next_transaction();
    
    // Queue initial transaction
    ESP_LOGI(TAG, "Queueing initial transaction with node ID 0x%02X", node_identifier);
    trans.length = SPI_MAX_TRANSFER_SIZE * 8;  // Length in bits
    trans.tx_buffer = spi_tx_buffer;
    trans.rx_buffer = spi_rx_buffer;
    trans.user = NULL;
    
    if (spi_slave_queue_trans(SPI_HOST, &trans, portMAX_DELAY) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to queue initial transaction");
    } else {
        ESP_LOGI(TAG, "Initial transaction queued successfully");
    }
    
    ESP_LOGI(TAG, "SPI receive task started");
    
    while (1) {
        // Wait for transaction to complete via the queue
        if (xQueueReceive(spi_event_queue, &trans, portMAX_DELAY)) {
            if (xSemaphoreTake(spi_mutex, portMAX_DELAY) == pdTRUE) {                
                ESP_LOGI(TAG, "SPI transaction completed");
                
                // Process received message
                handle_spi_message(spi_rx_buffer, spi_tx_buffer);
                
                // Queue next transaction with the prepared response
                trans.length = SPI_MAX_TRANSFER_SIZE * 8;  // Length in bits
                trans.tx_buffer = spi_tx_buffer;
                trans.rx_buffer = spi_rx_buffer;
                trans.user = NULL;

                ESP_LOGI(TAG, "SENDING MESSAGE:");
                for (int i = 0; i < 32; i += 8) {
                    ESP_LOGI(TAG, "  [%02X %02X %02X %02X %02X %02X %02X %02X]",
                             spi_tx_buffer[i], spi_tx_buffer[i + 1], spi_tx_buffer[i + 2], spi_tx_buffer[i + 3],
                             spi_tx_buffer[i + 4], spi_tx_buffer[i + 5], spi_tx_buffer[i + 6], spi_tx_buffer[i + 7]);
                }

                // Queue next transaction immediately
                esp_err_t ret = spi_slave_queue_trans(SPI_HOST, &trans, portMAX_DELAY);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to queue next transaction: %s", esp_err_to_name(ret));
                }
                
                xSemaphoreGive(spi_mutex);
            }
        }
    }
}

bool spi_slave_init(void) {
    ESP_LOGI(TAG, "Initializing SPI slave");
    
    // Create queue for SPI events
    spi_event_queue = xQueueCreate(10, sizeof(spi_slave_transaction_t *));
    if (spi_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create SPI event queue");
        return false;
    }
    
    // Create mutex for SPI operations
    spi_mutex = xSemaphoreCreateMutex();
    if (spi_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create SPI mutex");
        vQueueDelete(spi_event_queue);
        return false;
    }
    
    // Configure SPI slave interface
    spi_bus_config_t bus_config = {
        .mosi_io_num = SPI_MOSI_PIN,
        .miso_io_num = SPI_MISO_PIN,
        .sclk_io_num = SPI_SCLK_PIN,
        .quadwp_io_num = -1,  // Not used
        .quadhd_io_num = -1,  // Not used
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
        .flags = 0,
        .intr_flags = 0
    };
    
    spi_slave_interface_config_t slave_config = {
        .mode = 0,  // SPI mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = SPI_CS_PIN,
        .queue_size = 1,
        .flags = 0,  // No special flags
        .post_setup_cb = NULL,
        .post_trans_cb = spi_post_trans_callback
    };
    
    // Initialize SPI slave driver
    esp_err_t ret = spi_slave_initialize(SPI_HOST, &bus_config, &slave_config, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI slave: %s", esp_err_to_name(ret));
        vSemaphoreDelete(spi_mutex);
        vQueueDelete(spi_event_queue);
        return false;
    }
    
    // Prepare initial response buffer
    prepare_next_transaction();
    
    ESP_LOGI(TAG, "Initial response buffer prepared with type 0x%02X (decimal %d), identifier: 0x%02X",
             spi_tx_buffer[0], spi_tx_buffer[0], node_identifier);
    
    ESP_LOGI(TAG, "SPI slave initialized successfully, node identifier: 0x%02X", node_identifier);
    
    return true;
}

void spi_slave_deinit(void) {
    if (spi_task_handle != NULL) {
        vTaskDelete(spi_task_handle);
        spi_task_handle = NULL;
    }
    
    spi_slave_free(SPI_HOST);
    
    if (spi_mutex != NULL) {
        vSemaphoreDelete(spi_mutex);
        spi_mutex = NULL;
    }
    
    if (spi_event_queue != NULL) {
        vQueueDelete(spi_event_queue);
        spi_event_queue = NULL;
    }
    
    ESP_LOGI(TAG, "SPI slave deinitialized");
}

bool spi_slave_start_tasks(void) {
    ESP_LOGI(TAG, "Starting SPI receive tasks...");
    
    // Create SPI task
    BaseType_t ret = xTaskCreate(spi_task, "spi_slave_task", 4096, NULL, 5, &spi_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create SPI task");
        return false;
    }
    
    return true;
}

uint8_t spi_slave_get_wifi_channel(void) {
    return wifi_channel;
}
