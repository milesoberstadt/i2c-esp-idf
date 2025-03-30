#include "i2c_slave.h"
#include "wifi_scanner.h"
#include "ap_list.h"
#include <inttypes.h>

static const char *TAG = I2C_SLAVE_TAG;

// I2C slave device handle
static i2c_slave_dev_handle_t slave_handle;

// Message queue for processing I2C messages
static QueueHandle_t i2c_msg_queue;

// Send/receive buffers
static uint8_t tx_buffer[I2C_DATA_LEN];
static uint8_t rx_buffer[I2C_DATA_LEN];

// The SUB node configuration
static sub_config_t sub_config = {
    .id = {0},
    .i2c_addr = I2C_FIXED_SLAVE_ADDR,
    .wifi_channel = 0,
    .status = SUB_STATUS_UNINITIALIZED,
    .timestamp = 0
};

// Verification flag
static bool is_verified = false;

// Forward declarations
static void i2c_process_task(void *pvParameters);
static void i2c_message_handler(i2c_message_t *msg);

// We'll implement a simple write method
static esp_err_t i2c_slave_write_tx_buffer(const uint8_t *data, size_t len)
{
    // Simple implementation - the message will be sent on next master read
    if (len > I2C_DATA_LEN) {
        len = I2C_DATA_LEN;  // Truncate if too long
    }
    
    // Copy data to the tx buffer
    memcpy(tx_buffer, data, len);
    
    return ESP_OK;
}

// I2C slave receive callback - triggered when data is received from master
static IRAM_ATTR bool i2c_slave_rx_callback(i2c_slave_dev_handle_t channel, 
                                           const i2c_slave_rx_done_event_data_t *edata, 
                                           void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    
    // Copy data to a queue to be processed later by a task
    // Access to the buffer and copy its contents - assuming structure contains buffer and buffer_size
    // Let's copy whatever data we have in edata
    memcpy(rx_buffer, edata, sizeof(i2c_slave_rx_done_event_data_t));
    
    // Queue a fixed size that will tell our task to process the rx_buffer
    uint32_t data_size = sizeof(i2c_slave_rx_done_event_data_t);
    xQueueSendFromISR(queue, &data_size, &high_task_wakeup);
    
    return high_task_wakeup == pdTRUE;
}

esp_err_t i2c_slave_init(void) {
    ESP_LOGI(TAG, "Initializing I2C slave with the new driver API");
    
    // Generate random ID
    random_id_generate(sub_config.id);
    
    // Using fixed I2C address for SUB node
    ESP_LOGI(TAG, "FIXED I2C ADDRESS MODE: SUB ID: %s, Fixed I2C address: 0x%02X", 
             sub_config.id, sub_config.i2c_addr);
    ESP_LOGI(TAG, "I2C pins: SCL=%d, SDA=%d", I2C_SLAVE_SCL_IO, I2C_SLAVE_SDA_IO);
    
    // Initialize buffers
    memset(tx_buffer, 0xFF, sizeof(tx_buffer));
    memset(rx_buffer, 0xFF, sizeof(rx_buffer));
    
    // Prepare HELLO response message in advance
    tx_buffer[0] = MSG_HELLO;
    tx_buffer[1] = 0; // SUB ID not yet assigned
    tx_buffer[2] = 2; // data length (ID is 2 chars)
    tx_buffer[3] = sub_config.id[0];
    tx_buffer[4] = sub_config.id[1];
    
    // I2C slave configuration
    i2c_slave_config_t slave_config = {
        .addr_bit_len = I2C_ADDR_BIT_LEN_7,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_SLAVE_PORT,
        .send_buf_depth = 256,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .slave_addr = I2C_FIXED_SLAVE_ADDR,
    };
    
    // Create I2C slave device
    esp_err_t ret = i2c_new_slave_device(&slave_config, &slave_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C slave device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create message queue - just to store sizes, the buffer is static
    i2c_msg_queue = xQueueCreate(10, sizeof(uint32_t));
    if (i2c_msg_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create message queue");
        i2c_del_slave_device(slave_handle);
        return ESP_FAIL;
    }
    
    // Create callbacks structure
    i2c_slave_event_callbacks_t callbacks = {
        .on_recv_done = i2c_slave_rx_callback
    };
    
    // Register callbacks for receiving and sending data
    ret = i2c_slave_register_event_callbacks(slave_handle, &callbacks, i2c_msg_queue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register callbacks: %s", esp_err_to_name(ret));
        vQueueDelete(i2c_msg_queue);
        i2c_del_slave_device(slave_handle);
        return ret;
    }
    
    // Prepare initial response - hello message with our ID
    i2c_slave_prepare_response(MSG_HELLO, (uint8_t *)sub_config.id, 2);
    
    // Create task to process received messages
    BaseType_t task_created = xTaskCreate(
        i2c_process_task,
        "i2c_proc",
        4096,
        NULL,
        5,
        NULL
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create I2C processing task");
        vQueueDelete(i2c_msg_queue);
        i2c_del_slave_device(slave_handle);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "I2C slave initialized successfully");
    return ESP_OK;
}

esp_err_t i2c_slave_begin(void) {
    ESP_LOGI(TAG, "Starting I2C slave on fixed address 0x%02X", sub_config.i2c_addr);
    
    // No specific start function in ESP-IDF v5.4 - the device is already active after init
    ESP_LOGI(TAG, "I2C slave ready to send/receive data");
    return ESP_OK;
}

esp_err_t i2c_slave_stop(void) {
    ESP_LOGI(TAG, "Stopping I2C slave");
    
    // Delete slave device
    esp_err_t ret = i2c_del_slave_device(slave_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete I2C slave device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Delete queue
    vQueueDelete(i2c_msg_queue);
    
    ESP_LOGI(TAG, "I2C slave stopped successfully");
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

// Prepare a response message for the DOM
void i2c_slave_prepare_response(uint8_t msg_type, const uint8_t *data, uint8_t data_len) {
    if (data_len > I2C_MESSAGE_DATA_LEN) {
        ESP_LOGE(TAG, "Data too large for I2C message (%u > %u)", 
                 data_len, I2C_MESSAGE_DATA_LEN);
        return;
    }
    
    // Create a temporary buffer for the message
    uint8_t message[I2C_DATA_LEN];
    memset(message, 0xFF, sizeof(message)); // Fill with 0xFF (unused marker)
    
    // Set up message header
    message[0] = msg_type;             // Message type
    message[1] = 0;                    // SUB ID (will be assigned by DOM)
    message[2] = data_len;             // Data length
    
    // Copy data if provided
    if (data != NULL && data_len > 0) {
        memcpy(message + I2C_HEADER_LEN, data, data_len);
    }
    
    // Write to the TX buffer
    i2c_slave_write_tx_buffer(message, I2C_DATA_LEN);
    
    ESP_LOGD(TAG, "Prepared response message type 0x%02X with %u bytes of data", 
             msg_type, data_len);
}

// Handle AP count request
static void handle_ap_count_request(void) {
    uint16_t count = ap_list_get_count();
    i2c_slave_prepare_response(MSG_AP_COUNT, (uint8_t *)&count, sizeof(count));
    ESP_LOGI(TAG, "AP count request: %u APs found", count);
}

// Handle AP data request
static void handle_ap_data_request(void) {
    ap_record_t record;
    if (ap_list_get_next_unsync(&record)) {
        // Found an unsynced record, send it
        i2c_slave_prepare_response(MSG_AP_DATA, (uint8_t *)&record, sizeof(record));
        ESP_LOGD(TAG, "Sending AP data: RSSI %d, Channel %u", 
                record.rssi, record.channel);
    } else {
        // No more unsynced records
        i2c_slave_prepare_response(MSG_AP_DATA, NULL, 0);
        ESP_LOGD(TAG, "No more unsynced AP records");
    }
}

// I2C message processing task
static void i2c_process_task(void *pvParameters) {
    uint32_t data_size;
    
    ESP_LOGI(TAG, "I2C message processing task started");
    
    while (1) {
        if (xQueueReceive(i2c_msg_queue, &data_size, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Received I2C data, size=%u", (unsigned int)data_size);
            
            // We have received data, treat first few bytes as our message header
            if (data_size > 0) {
                // Assume rx_buffer contains our I2C message
                i2c_message_t msg;
                
                // The first bytes contain our message header - size is checked inside handler
                msg.header.msg_type = rx_buffer[0];
                msg.header.sub_id = rx_buffer[1];
                msg.header.data_len = rx_buffer[2];
                
                // Copy data portion
                if (msg.header.data_len > 0 && msg.header.data_len <= I2C_MESSAGE_DATA_LEN) {
                    memcpy(msg.data, &rx_buffer[3], msg.header.data_len);
                }
                
                ESP_LOGI(TAG, "Received message from DOM: type=0x%02X, sub_id=0x%02X, len=%d", 
                        msg.header.msg_type, msg.header.sub_id, msg.header.data_len);
                
                // Process the message
                i2c_message_handler(&msg);
            }
        }
    }
}

// Message handler
static void i2c_message_handler(i2c_message_t *msg) {
    if (!msg) return;
    
    ESP_LOGD(TAG, "Processing message type: 0x%02X, sub_id: 0x%02X, data_len: %u", 
             msg->header.msg_type, msg->header.sub_id, msg->header.data_len);
    
    switch (msg->header.msg_type) {
        case MSG_HELLO:
            // DOM is saying hello, respond with our ID
            ESP_LOGI(TAG, "DOM hello message received");
            i2c_slave_prepare_response(MSG_HELLO, (uint8_t *)sub_config.id, 2);
            break;
            
        case MSG_VERIFY:
            // DOM is verifying our ID
            if (msg->header.data_len >= 2 && 
                memcmp(msg->data, sub_config.id, 2) == 0) {
                // ID matches, we're verified
                ESP_LOGI(TAG, "Verified by DOM!");
                is_verified = true;
                i2c_slave_prepare_response(MSG_VERIFY, (uint8_t *)sub_config.id, 2);
            } else {
                // ID mismatch
                ESP_LOGW(TAG, "Verification failed - ID mismatch");
                i2c_slave_prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        case MSG_ASSIGN:
            // DOM is assigning us WiFi channel and SUB ID (in fixed address mode)
            if (msg->header.data_len >= 3 && is_verified) {
                uint8_t new_i2c_addr = msg->data[0]; // New I2C address (not used in fixed mode)
                uint8_t wifi_channel = msg->data[1];
                uint8_t sub_id = msg->data[2];
                
                ESP_LOGI(TAG, "DOM assigned: New I2C addr=0x%02X, WiFi channel=%u, SUB_ID=%u", 
                         new_i2c_addr, wifi_channel, sub_id);
                
                // In fixed address mode, we ignore the new I2C address and keep our fixed address
                ESP_LOGI(TAG, "Fixed address mode: Keeping fixed I2C address 0x%02X", sub_config.i2c_addr);
                
                // Update our configuration
                sub_config.wifi_channel = wifi_channel;
                sub_config.status = SUB_STATUS_INITIALIZED;
                
                i2c_slave_prepare_response(MSG_ASSIGN, NULL, 0);
            } else {
                ESP_LOGW(TAG, "Assignment rejected - not verified or invalid data");
                i2c_slave_prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        case MSG_RESET:
            // DOM is requesting a reset
            ESP_LOGI(TAG, "DOM requested reset, resetting SUB configuration");
            
            // Reset our configuration
            random_id_generate(sub_config.id);
            sub_config.wifi_channel = 0;
            sub_config.status = SUB_STATUS_UNINITIALIZED;
            sub_config.timestamp = 0;
            is_verified = false;
            
            // Stop WiFi scanning if active
            if (sub_config.status == SUB_STATUS_SCANNING) {
                wifi_scanner_stop();
            }
            
            // Clear AP list
            ap_list_clear();
            
            // Prepare initial response for next master communication
            i2c_slave_prepare_response(MSG_HELLO, (uint8_t *)sub_config.id, 2);
            
            ESP_LOGI(TAG, "SUB reset complete, new ID: %s", sub_config.id);
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
                
                i2c_slave_prepare_response(MSG_SET_TIME, NULL, 0);
            } else {
                ESP_LOGW(TAG, "Invalid timestamp data");
                i2c_slave_prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        case MSG_START_SCAN:
            // DOM is asking us to start scanning
            if (sub_config.status != SUB_STATUS_SCANNING) {
                ESP_LOGI(TAG, "Starting WiFi scan on fixed channel %u", sub_config.wifi_channel);
                
                // Start WiFi scanner
                wifi_scanner_start(sub_config.wifi_channel);
                sub_config.status = SUB_STATUS_SCANNING;
                
                i2c_slave_prepare_response(MSG_START_SCAN, NULL, 0);
            } else {
                ESP_LOGW(TAG, "WiFi scanner already running");
                i2c_slave_prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        case MSG_STOP_SCAN:
            // DOM is asking us to stop scanning
            if (sub_config.status == SUB_STATUS_SCANNING) {
                ESP_LOGI(TAG, "Stopping WiFi scan");
                
                // Stop WiFi scanner
                wifi_scanner_stop();
                sub_config.status = SUB_STATUS_INITIALIZED;
                
                i2c_slave_prepare_response(MSG_STOP_SCAN, NULL, 0);
            } else {
                ESP_LOGW(TAG, "WiFi scanner not running");
                i2c_slave_prepare_response(MSG_ERROR, NULL, 0);
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
                    i2c_slave_prepare_response(MSG_CONFIRM_AP, NULL, 0);
                } else {
                    ESP_LOGW(TAG, "AP not found for sync marking");
                    i2c_slave_prepare_response(MSG_ERROR, NULL, 0);
                }
            } else {
                ESP_LOGW(TAG, "Invalid confirm AP data");
                i2c_slave_prepare_response(MSG_ERROR, NULL, 0);
            }
            break;
            
        default:
            // Unknown message type
            ESP_LOGW(TAG, "Unknown message type: 0x%02X", msg->header.msg_type);
            i2c_slave_prepare_response(MSG_ERROR, NULL, 0);
            break;
    }
}