#include "i2c_slave.h"
#include "esp_random.h"

i2c_slave_dev_handle_t slave_handle;
QueueHandle_t s_receive_queue;
uint8_t *data_rd;

// Define the global variables
uint8_t i2c_slave_addr;     // Dynamic I2C slave address
uint8_t device_identifier;  // Random device identifier (single byte)

static IRAM_ATTR bool i2c_slave_rx_done_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

void i2c_slave_init() {
    ESP_LOGI(I2C_SLAVE_TAG, "Initializing I2C slave");
    
    // Generate random I2C slave address between MIN and MAX (inclusive)
    i2c_slave_addr = (esp_random() % (I2C_SLAVE_ADDR_MAX - I2C_SLAVE_ADDR_MIN + 1)) + I2C_SLAVE_ADDR_MIN;
    
    // Generate random 1-byte device identifier (0x00 - 0xFF)
    device_identifier = esp_random() % 0x100;
    
    ESP_LOGI(I2C_SLAVE_TAG, "Generated random device ID: 0x%02X", device_identifier);
    ESP_LOGI(I2C_SLAVE_TAG, "Using I2C slave address: 0x%02X", i2c_slave_addr);
    
    data_rd = (uint8_t *) malloc(I2C_DATA_LEN);
    if (data_rd == NULL) {
        ESP_LOGE(I2C_SLAVE_TAG, "Failed to allocate memory for receive buffer");
        return;
    }

    i2c_slave_config_t i2c_slv_config = {
        .addr_bit_len = I2C_ADDR_BIT_LEN_7,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .send_buf_depth = 256,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .slave_addr = i2c_slave_addr,  // Use the random address
    };

    esp_err_t ret = i2c_new_slave_device(&i2c_slv_config, &slave_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_SLAVE_TAG, "I2C slave device creation failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(I2C_SLAVE_TAG, "I2C slave device created successfully");

    s_receive_queue = xQueueCreate(8, sizeof(i2c_slave_rx_done_event_data_t));
    if (s_receive_queue == NULL) {
        ESP_LOGE(I2C_SLAVE_TAG, "Failed to create I2C receive queue");
        return;
    }
    
    i2c_slave_event_callbacks_t cbs = {
        .on_recv_done = i2c_slave_rx_done_callback,
    };
    ret = i2c_slave_register_event_callbacks(slave_handle, &cbs, s_receive_queue);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_SLAVE_TAG, "I2C slave callback registration failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(I2C_SLAVE_TAG, "I2C slave callbacks registered successfully");

    ESP_LOGI(I2C_SLAVE_TAG, "I2C slave initialized");
}

void i2c_receive_task(void *pvParameters) {
    int error_count = 0;
    const int max_consecutive_errors = 5;
    
    ESP_LOGI(I2C_SLAVE_TAG, "I2C receive task started");
    
    while (1) {
        ESP_LOGD(I2C_SLAVE_TAG, "Waiting for I2C data...");

        // Clear buffer before receiving data
        memset(data_rd, 0, I2C_DATA_LEN);

        esp_err_t ret = i2c_slave_receive(slave_handle, data_rd, I2C_DATA_LEN);
        
        if (ret == ESP_OK) {
            // Successful receive - just keep count of successes at debug level
            ESP_LOGD(I2C_SLAVE_TAG, "Data received successfully from master");
            error_count = 0;  // Reset error counter on success
        } else {
            error_count++;
            ESP_LOGE(I2C_SLAVE_TAG, "Error receiving data: %s (error %d/%d)", 
                     esp_err_to_name(ret), error_count, max_consecutive_errors);
            
            if (error_count >= max_consecutive_errors) {
                ESP_LOGW(I2C_SLAVE_TAG, "Too many consecutive errors, reinitializing I2C slave...");
                // Could add reinitialization logic here if needed
                error_count = 0;
            }
            
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
    }
}

extern void process_message(uint8_t* data, size_t length);

void i2c_process_queue_task(void *pvParameters) {
    i2c_slave_rx_done_event_data_t rx_data;
    
    while (1) {
        if (xQueueReceive(s_receive_queue, &rx_data, portMAX_DELAY) == pdTRUE) {
            // Process the received message
            uint8_t buf[I2C_DATA_LEN];
            memcpy(buf, rx_data.buffer, I2C_DATA_LEN);
            process_message(buf, I2C_DATA_LEN);
        }
    }
}

void i2c_start_receive() {
    ESP_LOGI(I2C_SLAVE_TAG, "Starting I2C receive tasks");
    xTaskCreate(i2c_receive_task, "i2c_receive_task", 4096, NULL, 5, NULL);
    xTaskCreate(i2c_process_queue_task, "i2c_process_queue_task", 4096, NULL, 5, NULL);
}

bool i2c_slave_send(uint8_t* data, size_t length) {
    if (data == NULL || length == 0) {
        ESP_LOGE(I2C_SLAVE_TAG, "Invalid data to send");
        return false;
    }

    ESP_LOGI(I2C_SLAVE_TAG, "Preparing to send data to master");

    // Check if this is an identifier message (special handling)
    if (data[0] == msg_res_identifier) { // msg_res_identifier value
        ESP_LOGD(I2C_SLAVE_TAG, "Sending identifier response message");
        ESP_LOGD(I2C_SLAVE_TAG, "Message header: %02X %02X %02X", data[0], data[1], data[2]);
        ESP_LOGD(I2C_SLAVE_TAG, "Identifier data: %02X %02X", data[3], data[4]);
    }

    // Log the first few bytes we're sending
    ESP_LOGD(I2C_SLAVE_TAG, "Sending message with first bytes: %02X %02X %02X %02X",
            data[0], data[1], data[2], data[3]);

    // Make multiple transmission attempts if needed
    int max_attempts = 3;
    esp_err_t ret = ESP_FAIL;

    // if we want to fail back to single attempts, here was the working code
    // ESP_LOGI(I2C_SLAVE_TAG, "Sending data to master");
    // esp_err_t ret = i2c_slave_transmit(slave_handle, data, length, portMAX_DELAY);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(I2C_SLAVE_TAG, "Error sending data: %s", esp_err_to_name(ret));
    //     return false;
    // }
    // ESP_LOGI(I2C_SLAVE_TAG, "Data sent successfully");
    // return true;

    for (int i = 0; i < max_attempts; i++) {
        ret = i2c_slave_transmit(slave_handle, data, length, portMAX_DELAY);

        if (ret == ESP_OK) {
            ESP_LOGI(I2C_SLAVE_TAG, "Data sent successfully on attempt %d", i+1);

            // Add a small delay to ensure master can process the message
            vTaskDelay(pdMS_TO_TICKS(10));
            return true;
        } else {
            ESP_LOGE(I2C_SLAVE_TAG, "Error sending data (attempt %d/%d): %s",
                    i+1, max_attempts, esp_err_to_name(ret));

            // Wait a bit before retrying
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    
    ESP_LOGE(I2C_SLAVE_TAG, "Failed to send data after %d attempts", max_attempts);
    return false;
}