#include "i2c_master.h"
#include "i2c_messages.h"
#include "types.h"

i2c_master_dev_handle_t dev_handle;
i2c_master_bus_handle_t bus_handle;

// Define the slave device information
uint8_t discovered_slave_addr = 0;
uint16_t slave_identifier = 0;

// Create temporary device handle for scanning
i2c_master_dev_handle_t temp_dev_handle = NULL;

bool i2c_init() {
    ESP_LOGI(I2C_TAG, "Initializing I2C master on port %d, SDA: %d, SCL: %d", 
             I2C_PORT_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = 1,
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "I2C master bus creation failed: %s", esp_err_to_name(ret));
        return false;
    }
    ESP_LOGI(I2C_TAG, "I2C master bus created successfully");

    // Scan for slave
    if (!i2c_scan_for_slave()) {
        ESP_LOGE(I2C_TAG, "Failed to find I2C slave device");
        return false;
    }

    // Read slave identifier
    if (!i2c_read_slave_identifier()) {
        ESP_LOGW(I2C_TAG, "Failed to read slave identifier");
    }

    // Create the final device handle with the discovered address
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = discovered_slave_addr,
        .scl_speed_hz = 100000,
    };

    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "I2C master device addition failed: %s", esp_err_to_name(ret));
        return false;
    }
    ESP_LOGI(I2C_TAG, "I2C master device added successfully with address 0x%02X", discovered_slave_addr);
    ESP_LOGI(I2C_TAG, "I2C slave identifier: 0x%02X", slave_identifier);

    ESP_LOGI(I2C_TAG, "I2C master initialized successfully");
    return true;
}

bool i2c_scan_for_slave() {
    ESP_LOGI(I2C_TAG, "Scanning for I2C slave device in range 0x%02X - 0x%02X", 
             I2C_SLAVE_ADDR_MIN, I2C_SLAVE_ADDR_MAX);

    uint8_t device_count = 0;
    
    for (uint8_t addr = I2C_SLAVE_ADDR_MIN; addr <= I2C_SLAVE_ADDR_MAX; addr++) {
        ESP_LOGD(I2C_TAG, "Probing address 0x%02X...", addr);

        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = 100000,
        };

        // Free the temporary device handle if it's not NULL
        if (temp_dev_handle != NULL) {
            i2c_master_bus_rm_device(temp_dev_handle);
            temp_dev_handle = NULL;
        }

        // Try to add the device at this address
        esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &temp_dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGD(I2C_TAG, "Failed to add device at address 0x%02X", addr);
            continue;
        }

        // Try to write 0 bytes to detect if a device is present
        uint8_t dummy_data[1] = {0};
        ret = i2c_master_transmit(temp_dev_handle, dummy_data, 1, 10);
        
        if (ret == ESP_OK) {
            ESP_LOGI(I2C_TAG, "Device found at address 0x%02X", addr);
            device_count++;
            discovered_slave_addr = addr;
            
            // For now, we'll just take the first device we find
            // In a more complex setup, we could test if it responds to our protocol
            break;
        }
    }

    // Free the temporary device handle
    if (temp_dev_handle != NULL) {
        i2c_master_bus_rm_device(temp_dev_handle);
        temp_dev_handle = NULL;
    }

    if (device_count == 0) {
        ESP_LOGW(I2C_TAG, "No I2C slave devices found");
        return false;
    }

    ESP_LOGI(I2C_TAG, "Found I2C slave at address 0x%02X", discovered_slave_addr);
    return true;
}

bool i2c_read_slave_identifier() {
    ESP_LOGI(I2C_TAG, "Reading identifier from slave at address 0x%02X", discovered_slave_addr);

    // Add a device with the discovered address for communication
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = discovered_slave_addr,
        .scl_speed_hz = 100000,
    };

    // Create a temporary device for the identifier request
    i2c_master_dev_handle_t id_dev_handle;
    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &id_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "Failed to add device for identifier request: %s", esp_err_to_name(ret));
        return false;
    }

    // Request the identifier from the slave
    uint8_t req_data[I2C_DATA_LEN] = {0};
    req_data[0] = msg_req_identifier;  // Message type
    req_data[1] = 0;                   // Device index
    req_data[2] = 0;                   // Data length

    // Send the request
    ret = i2c_master_transmit(id_dev_handle, req_data, I2C_DATA_LEN, 100);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "Error sending identifier request: %s", esp_err_to_name(ret));
        i2c_master_bus_rm_device(id_dev_handle);
        return false;
    }

    // Delay to give slave time to process the request
    vTaskDelay(pdMS_TO_TICKS(50));

    // Receive the response
    uint8_t res_data[I2C_DATA_LEN] = {0};
    ret = i2c_master_receive(id_dev_handle, res_data, I2C_DATA_LEN, 100);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "Error receiving identifier response: %s", esp_err_to_name(ret));
        i2c_master_bus_rm_device(id_dev_handle);
        return false;
    }

    // Check if the response is the expected type
    if (res_data[0] != msg_res_identifier) {
        ESP_LOGE(I2C_TAG, "Unexpected response type: %d", res_data[0]);
        i2c_master_bus_rm_device(id_dev_handle);
        return false;
    }

    // Extract the identifier from the response (2 bytes)
    uint8_t data_len = res_data[2];
    if (data_len >= 2) {
        slave_identifier = (res_data[3] << 8) | res_data[4];
        ESP_LOGI(I2C_TAG, "Slave identifier: 0x%02X", slave_identifier);
    } else {
        ESP_LOGW(I2C_TAG, "Invalid identifier data length: %d", data_len);
        i2c_master_bus_rm_device(id_dev_handle);
        return false;
    }

    // Clean up the temporary device
    i2c_master_bus_rm_device(id_dev_handle);
    return true;
}

bool i2c_write(uint8_t *data_wr) {
    esp_err_t ret = i2c_master_transmit(dev_handle, data_wr, I2C_DATA_LEN, 100);

    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "Error sending data: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(I2C_TAG, "Data sent successfully");
    return true;
}