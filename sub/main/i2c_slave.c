#include "i2c_slave.h"
#include "wifi_scanner.h"
#include "ap_list.h"
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_random.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "i2c_slave";

// I2C pins
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define I2C_PORT I2C_NUM_0

// Maximum message queue size
#define I2C_QUEUE_SIZE 10

// The sub node configuration
static sub_config_t sub_config = {
    .id = {0},
    .i2c_addr = 0,
    .wifi_channel = 0,
    .status = SUB_STATUS_UNINITIALIZED,
    .timestamp = 0
};

// Verification flag
static bool is_verified = false;

// Message queue for processing messages
static QueueHandle_t i2c_msg_queue = NULL;

// Task handle for message processing
static TaskHandle_t i2c_process_task_handle = NULL;

// Buffer for receiving I2C data
static uint8_t i2c_data_buffer[I2C_DATA_LEN];
static uint8_t i2c_send_buffer[I2C_DATA_LEN];

// Forward declarations for internal functions
static void i2c_process_task(void *pvParameters);
static void i2c_message_handler(i2c_message_t *msg);
static void prepare_response(uint8_t msg_type, uint8_t *data, uint8_t data_len);

// I2C event handler - this implementation uses the older i2c API
// Dedicated task for handling I2C slave events
static void i2c_slave_task(void *pvParameters) {
    i2c_message_t msg;
    uint8_t recv_buf[I2C_DATA_LEN];
    int len;
    
    ESP_LOGI(TAG, "I2C slave task started on address 0x%02X", sub_config.i2c_addr);
    
    // Prepare initial response - a hello message with our ID
    prepare_response(MSG_HELLO, (uint8_t *)sub_config.id, 2);
    
    while (1) {
        // Always make sure our response buffer is ready
        i2c_slave_write_buffer(I2C_PORT, i2c_send_buffer, sizeof(i2c_send_buffer), 0);
        
        // Check if master is trying to write to us
        len = i2c_slave_read_buffer(I2C_PORT, recv_buf, sizeof(recv_buf), 0);
        
        if (len > 0) {
            // Received a message from master (DOM)
            memcpy(&msg, recv_buf, sizeof(msg));
            
            ESP_LOGI(TAG, "Received message from DOM: type=0x%02X, sub_id=0x%02X, len=%d", 
                    msg.header.msg_type, msg.header.sub_id, msg.header.data_len);
            
            // Process message and prepare response immediately
            i2c_message_handler(&msg);
            
            // Send to processing queue for background handling
            if (xQueueSend(i2c_msg_queue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
                ESP_LOGW(TAG, "Failed to queue I2C message");
            }
            
            // Small delay after processing a message to ensure response is ready
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        
        // Small delay to prevent high CPU usage
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

esp_err_t i2c_slave_init(void) {
    ESP_LOGI(TAG, "Initializing I2C slave");
    
    // Generate random ID
    random_id_generate(sub_config.id);
    
    // Generate random I2C address in discovery range
    sub_config.i2c_addr = I2C_ADDR_RANDOM_MIN + (esp_random() % (I2C_ADDR_RANDOM_MAX - I2C_ADDR_RANDOM_MIN + 1));
    
    // Initialize status
    sub_config.status = SUB_STATUS_UNINITIALIZED;
    sub_config.wifi_channel = 0;
    sub_config.timestamp = 0;
    
    ESP_LOGI(TAG, "SUB ID: %s, I2C address: 0x%02X", sub_config.id, sub_config.i2c_addr);
    
    // Create message queue
    i2c_msg_queue = xQueueCreate(I2C_QUEUE_SIZE, sizeof(i2c_message_t));
    if (i2c_msg_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create message queue");
        return ESP_FAIL;
    }
    
    // Initialize buffer with 0xFF (unused data marker)
    memset(i2c_data_buffer, 0xFF, sizeof(i2c_data_buffer));
    memset(i2c_send_buffer, 0xFF, sizeof(i2c_send_buffer));
    
    // I2C bus config
    i2c_config_t i2c_bus_config = {
        .mode = I2C_MODE_SLAVE,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .slave.addr_10bit_en = 0,
        .slave.slave_addr = sub_config.i2c_addr
    };
    
    // Initialize I2C bus
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &i2c_bus_config));
    
    // Initialize I2C driver
    // Using older API: i2c_driver_install(i2c_port_t, i2c_mode_t, size_t rx_buf_len, size_t tx_buf_len, int flags)
    // For slave mode, ESP-IDF requires larger buffer sizes (minimum 32 bytes for RX and TX)
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_SLAVE, 128, 128, 0));
    
    // Create message processing task
    BaseType_t task_created = xTaskCreate(
        i2c_process_task,
        "i2c_proc",
        4096,
        NULL,
        5,
        &i2c_process_task_handle
    );
    
    // Create I2C slave handling task
    static TaskHandle_t i2c_slave_task_handle = NULL;
    BaseType_t i2c_task_created = xTaskCreate(
        i2c_slave_task,
        "i2c_slave",
        4096,
        NULL,
        6, // Higher priority than processing task
        &i2c_slave_task_handle
    );
    
    if (task_created != pdPASS || i2c_task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create I2C tasks");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "I2C slave initialized");
    return ESP_OK;
}

esp_err_t i2c_slave_start(void) {
    ESP_LOGI(TAG, "Starting I2C slave on address 0x%02X", sub_config.i2c_addr);
    return ESP_OK;
}

esp_err_t i2c_slave_stop(void) {
    ESP_LOGI(TAG, "Stopping I2C slave");
    
    // Delete task if it exists
    if (i2c_process_task_handle != NULL) {
        vTaskDelete(i2c_process_task_handle);
        i2c_process_task_handle = NULL;
    }
    
    // Delete queue if it exists
    if (i2c_msg_queue != NULL) {
        vQueueDelete(i2c_msg_queue);
        i2c_msg_queue = NULL;
    }
    
    // Uninstall I2C driver
    ESP_ERROR_CHECK(i2c_driver_delete(I2C_PORT));
    
    return ESP_OK;
}

esp_err_t i2c_slave_set_address(uint8_t new_addr) {
    if (new_addr < I2C_ADDR_RESERVED_MIN || new_addr > I2C_ADDR_RESERVED_MAX) {
        ESP_LOGE(TAG, "Invalid I2C address: 0x%02X", new_addr);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Changing I2C address from 0x%02X to 0x%02X", sub_config.i2c_addr, new_addr);
    
    // Stop I2C slave
    i2c_slave_stop();
    
    // Update address
    sub_config.i2c_addr = new_addr;
    
    // Reinitialize with new address
    i2c_slave_init();
    i2c_slave_start();
    
    return ESP_OK;
}

sub_config_t* i2c_slave_get_config(void) {
    return &sub_config;
}

bool i2c_slave_is_verified(void) {
    return is_verified;
}

void i2c_slave_reset_verification(void) {
    is_verified = false;
}

// Prepare a response message to the DOM
static void prepare_response(uint8_t msg_type, uint8_t *data, uint8_t data_len) {
    // Clear send buffer
    memset(i2c_send_buffer, 0xFF, sizeof(i2c_send_buffer));
    
    // Set up message header
    i2c_message_t *msg = (i2c_message_t *)i2c_send_buffer;
    msg->header.msg_type = msg_type;
    msg->header.sub_id = 0; // Will be assigned by DOM
    msg->header.data_len = data_len;
    
    // Copy data if provided
    if (data != NULL && data_len > 0) {
        memcpy(msg->data, data, data_len);
    }
}

// Handle AP count request
static void handle_ap_count_request(void) {
    uint16_t count = ap_list_get_count();
    prepare_response(MSG_AP_COUNT, (uint8_t *)&count, sizeof(count));
    ESP_LOGI(TAG, "AP count request: %u APs found", count);
}

// Handle AP data request
static void handle_ap_data_request(void) {
    ap_record_t record;
    if (ap_list_get_next_unsync(&record)) {
        // Found an unsynced record, send it
        prepare_response(MSG_AP_DATA, (uint8_t *)&record, sizeof(record));
        ESP_LOGD(TAG, "Sending AP data: RSSI %d, Channel %u", 
                record.rssi, record.channel);
    } else {
        // No more unsynced records
        prepare_response(MSG_AP_DATA, NULL, 0);
        ESP_LOGD(TAG, "No more unsynced AP records");
    }
}

// I2C message processing task
static void i2c_process_task(void *pvParameters) {
    i2c_message_t msg;
    
    ESP_LOGI(TAG, "I2C message processing task started");
    
    while (1) {
        // Wait for a message
        if (xQueueReceive(i2c_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
            i2c_message_handler(&msg);
        }
    }
}

// Message handler
static void i2c_message_handler(i2c_message_t *msg) {
    if (!msg) return;
    
    ESP_LOGD(TAG, "Received message type: 0x%02X, sub_id: 0x%02X, data_len: %u", 
             msg->header.msg_type, msg->header.sub_id, msg->header.data_len);
    
    switch (msg->header.msg_type) {
        case MSG_HELLO:
            // DOM is saying hello, respond with our ID
            ESP_LOGI(TAG, "DOM hello message received");
            prepare_response(MSG_HELLO, (uint8_t *)sub_config.id, 2);
            break;
            
        case MSG_VERIFY:
            // DOM is verifying our ID
            if (msg->header.data_len >= 2 && 
                memcmp(msg->data, sub_config.id, 2) == 0) {
                // ID matches, we're verified
                ESP_LOGI(TAG, "Verified by DOM!");
                is_verified = true;
                prepare_response(MSG_VERIFY, (uint8_t *)sub_config.id, 2);
            } else {
                // ID mismatch
                ESP_LOGW(TAG, "Verification failed - ID mismatch");
                prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        case MSG_ASSIGN:
            // DOM is assigning us a new I2C address and WiFi channel
            if (msg->header.data_len >= 3 && is_verified) {
                uint8_t new_i2c_addr = msg->data[0];
                uint8_t new_wifi_channel = msg->data[1];
                uint8_t assigned_sub_id = msg->data[2];
                
                ESP_LOGI(TAG, "DOM assigned: I2C=0x%02X, WiFi=%u, SUB_ID=%u", 
                         new_i2c_addr, new_wifi_channel, assigned_sub_id);
                
                // Update our configuration
                sub_config.wifi_channel = new_wifi_channel;
                sub_config.status = SUB_STATUS_INITIALIZED;
                
                // We need to respond before changing I2C address
                prepare_response(MSG_ASSIGN, NULL, 0);
                
                // Schedule address change (will be applied after response is sent)
                vTaskDelay(pdMS_TO_TICKS(100));
                i2c_slave_set_address(new_i2c_addr);
            } else {
                ESP_LOGW(TAG, "Assignment rejected - not verified");
                prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        case MSG_RESET:
            // DOM is asking us to reset
            ESP_LOGI(TAG, "Reset command received from DOM");
            
            // Stop WiFi scanner if running
            if (sub_config.status == SUB_STATUS_SCANNING) {
                wifi_scanner_stop();
            }
            
            // Reset verification status
            is_verified = false;
            
            // Reset our state but keep our ID
            char saved_id[3];
            memcpy(saved_id, sub_config.id, 3);
            
            // Generate a new random I2C address
            uint8_t new_i2c_addr = I2C_ADDR_RANDOM_MIN + (esp_random() % (I2C_ADDR_RANDOM_MAX - I2C_ADDR_RANDOM_MIN + 1));
            
            sub_config.i2c_addr = new_i2c_addr;
            sub_config.wifi_channel = 0;
            sub_config.status = SUB_STATUS_UNINITIALIZED;
            
            // Restore our ID
            memcpy(sub_config.id, saved_id, 3);
            
            prepare_response(MSG_RESET, NULL, 0);
            
            // Schedule address change (will be applied after response is sent)
            vTaskDelay(pdMS_TO_TICKS(100));
            i2c_slave_set_address(new_i2c_addr);
            break;
            
        case MSG_SET_TIME:
            // DOM is setting our timestamp
            if (msg->header.data_len >= 4) {
                uint32_t timestamp;
                memcpy(&timestamp, msg->data, 4);
                
                ESP_LOGI(TAG, "DOM set timestamp: %" PRIu32, timestamp);
                
                // Update our timestamp
                sub_config.timestamp = timestamp;
                wifi_scanner_set_timestamp(timestamp);
                
                prepare_response(MSG_SET_TIME, NULL, 0);
            } else {
                ESP_LOGW(TAG, "Invalid timestamp data");
                prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        case MSG_START_SCAN:
            // DOM is asking us to start scanning
            if (sub_config.status != SUB_STATUS_SCANNING) {
                ESP_LOGI(TAG, "Starting WiFi scan on channel %u", sub_config.wifi_channel);
                
                // Start WiFi scanner
                wifi_scanner_start(sub_config.wifi_channel);
                sub_config.status = SUB_STATUS_SCANNING;
                
                prepare_response(MSG_START_SCAN, NULL, 0);
            } else {
                ESP_LOGW(TAG, "WiFi scanner already running");
                prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        case MSG_STOP_SCAN:
            // DOM is asking us to stop scanning
            if (sub_config.status == SUB_STATUS_SCANNING) {
                ESP_LOGI(TAG, "Stopping WiFi scan");
                
                // Stop WiFi scanner
                wifi_scanner_stop();
                sub_config.status = SUB_STATUS_INITIALIZED;
                
                prepare_response(MSG_STOP_SCAN, NULL, 0);
            } else {
                ESP_LOGW(TAG, "WiFi scanner not running");
                prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        case MSG_REQ_AP_COUNT:
            // DOM is requesting AP count
            handle_ap_count_request();
            break;
            
        case MSG_REQ_AP_DATA:
            // DOM is requesting the next AP data record
            handle_ap_data_request();
            break;
            
        case MSG_CONFIRM_AP:
            // DOM is confirming it received the AP data
            if (msg->header.data_len >= 6) {
                // Extract BSSID from the message
                uint8_t bssid[6];
                memcpy(bssid, msg->data, 6);
                
                // Mark the AP as synced
                if (ap_list_mark_synced(bssid)) {
                    ESP_LOGD(TAG, "AP marked as synced");
                    prepare_response(MSG_CONFIRM_AP, NULL, 0);
                } else {
                    ESP_LOGW(TAG, "AP not found for sync marking");
                    prepare_response(MSG_ERROR, NULL, 0);
                }
            } else {
                ESP_LOGW(TAG, "Invalid confirm AP data");
                prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        default:
            // Unknown message type
            ESP_LOGW(TAG, "Unknown message type: 0x%02X", msg->header.msg_type);
            prepare_response(MSG_ERROR, NULL, 0);
            break;
    }
}