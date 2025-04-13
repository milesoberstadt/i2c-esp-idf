#include "i2c_master.h"
#include "i2c_messages.h"
#include "types.h"
#include <sys/time.h>

// I2C master bus handle
i2c_master_bus_handle_t bus_handle;

// Temporary device handle for scanning
i2c_master_dev_handle_t temp_dev_handle = NULL;

// Array to hold all sub node information
sub_node_t sub_nodes[MAX_SUB_NODES];

// For backward compatibility with legacy code
uint8_t discovered_slave_addr = 0;
uint8_t slave_identifier = 0;
i2c_master_dev_handle_t dev_handle;

// Get current timestamp in milliseconds for activity tracking
static uint32_t get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

// Clean up device handles but keep the I2C bus
bool i2c_reset_devices() {
    ESP_LOGI(I2C_TAG, "Resetting I2C device connections...");
    
    // Remove all device handles but keep the bus
    for (int i = 0; i < MAX_SUB_NODES; i++) {
        if (sub_nodes[i].handle != NULL) {
            ESP_LOGI(I2C_TAG, "Removing I2C device handle for node %d (addr: 0x%02X)", 
                     i, sub_nodes[i].address);
            esp_err_t ret = i2c_master_bus_rm_device(sub_nodes[i].handle);
            if (ret != ESP_OK) {
                ESP_LOGW(I2C_TAG, "Error removing device handle for node %d: %s", 
                         i, esp_err_to_name(ret));
            }
            sub_nodes[i].handle = NULL;
        }
        
        // Reset node information but preserve identifiers
        uint8_t saved_identifier = sub_nodes[i].identifier;
        sub_nodes[i].address = 0;
        sub_nodes[i].status = SUB_NODE_DISCONNECTED;
        sub_nodes[i].last_seen = 0;
        
        // Only preserve identifiers for nodes that were previously connected
        if (saved_identifier != 0) {
            ESP_LOGI(I2C_TAG, "Preserving identifier 0x%02X for node %d", saved_identifier, i);
            sub_nodes[i].identifier = saved_identifier;
        }
    }
    
    // Clean up the temporary device handle if it exists
    if (temp_dev_handle != NULL) {
        ESP_LOGI(I2C_TAG, "Removing temporary I2C device handle");
        i2c_master_bus_rm_device(temp_dev_handle);
        temp_dev_handle = NULL;
    }
    
    ESP_LOGI(I2C_TAG, "I2C device connections reset complete");
    return true;
}

// Full deinitialization - only needed when completely shutting down I2C
bool i2c_deinit() {
    ESP_LOGI(I2C_TAG, "Deinitializing I2C master...");
    
    // First reset all devices
    i2c_reset_devices();
    
    // Then delete the bus handle
    if (bus_handle != NULL) {
        ESP_LOGI(I2C_TAG, "Deleting I2C master bus");
        esp_err_t ret = i2c_del_master_bus(bus_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(I2C_TAG, "Error deleting I2C master bus: %s", esp_err_to_name(ret));
            return false;
        }
        bus_handle = NULL;
    }
    
    ESP_LOGI(I2C_TAG, "I2C master deinitialization complete");
    return true;
}

bool i2c_init() {
    ESP_LOGI(I2C_TAG, "Initializing I2C master on port %d, SDA: %d, SCL: %d", 
             I2C_PORT_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    // First ensure any existing I2C bus is deinitialized
    if (bus_handle != NULL) {
        ESP_LOGW(I2C_TAG, "I2C bus already initialized, deinitializing first");
        if (!i2c_deinit()) {
            ESP_LOGE(I2C_TAG, "Failed to deinitialize existing I2C bus");
            return false;
        }
    }

    // Initialize sub nodes array
    for (int i = 0; i < MAX_SUB_NODES; i++) {
        sub_nodes[i].address = 0;
        sub_nodes[i].identifier = 0;
        sub_nodes[i].handle = NULL;
        sub_nodes[i].status = SUB_NODE_DISCONNECTED;
        sub_nodes[i].last_seen = 0;
    }

    // Configure and create the I2C master bus
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

    // Scan for slaves
    if (!i2c_scan_for_slaves()) {
        ESP_LOGE(I2C_TAG, "Failed to find any I2C slave devices");
        return false;
    }

    // Update legacy variables for compatibility
    int first_node = i2c_get_first_node_index();
    if (first_node >= 0) {
        discovered_slave_addr = sub_nodes[first_node].address;
        slave_identifier = sub_nodes[first_node].identifier;
        dev_handle = sub_nodes[first_node].handle;
    }

    ESP_LOGI(I2C_TAG, "I2C master initialized successfully with %d node(s)", i2c_get_connected_node_count());
    return true;
}

bool i2c_scan_for_slaves() {
    ESP_LOGI(I2C_TAG, "Scanning for I2C slave devices in initial range 0x%02X - 0x%02X", 
             I2C_SLAVE_ADDR_MIN, I2C_SLAVE_ADDR_MAX);
    ESP_LOGI(I2C_TAG, "Also scanning reserved address range 0x%02X - 0x%02X for pre-assigned SUBs", 
             I2C_ASSIGNED_ADDR_MIN, I2C_ASSIGNED_ADDR_MAX);

    int device_count = 0;
    uint16_t scanned_count = 0;
    int assigned_count = 0;
    int unassigned_count = 0;
    
    // First, scan the assigned address range to find SUBs with saved addresses
    ESP_LOGI(I2C_TAG, "Checking for SUBs with pre-assigned addresses");
    for (uint8_t addr = I2C_ASSIGNED_ADDR_MIN; addr <= I2C_ASSIGNED_ADDR_MAX; addr++) {
        scanned_count++;
        
        // Skip if we've already found the maximum number of devices
        if (device_count >= MAX_SUB_NODES) {
            ESP_LOGI(I2C_TAG, "Maximum number of devices (%d) found, stopping scan", MAX_SUB_NODES);
            break;
        }
        
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
            continue;
        }

        // Try to write 0 bytes to detect if a device is present
        uint8_t dummy_data[1] = {0};
        ret = i2c_master_transmit(temp_dev_handle, dummy_data, 1, 10);
        
        if (ret == ESP_OK) {
            ESP_LOGI(I2C_TAG, "Device found at ASSIGNED address 0x%02X - likely a pre-configured SUB", addr);
            
            // Add this device to our list
            sub_nodes[device_count].address = addr;
            sub_nodes[device_count].status = SUB_NODE_CONNECTED;
            sub_nodes[device_count].last_seen = get_current_time_ms();

            // Create a persistent device handle for this sub node
            i2c_device_config_t node_cfg = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = addr,
                .scl_speed_hz = 100000,
            };

            i2c_master_dev_handle_t node_handle;
            ret = i2c_master_bus_add_device(bus_handle, &node_cfg, &node_handle);
            if (ret != ESP_OK) {
                ESP_LOGE(I2C_TAG, "Failed to add device handle for pre-assigned sub node at address 0x%02X", addr);
                continue;
            }

            sub_nodes[device_count].handle = node_handle;
            
            // Try to read the identifier
            if (i2c_read_slave_identifier(device_count)) {
                ESP_LOGI(I2C_TAG, "Pre-assigned sub node %d: address=0x%02X, identifier=0x%02X", 
                        device_count, sub_nodes[device_count].address, sub_nodes[device_count].identifier);
                assigned_count++;
            } else {
                ESP_LOGW(I2C_TAG, "Failed to read identifier for pre-assigned sub node at address 0x%02X", addr);
            }
            
            device_count++;
        }
    }
    
    ESP_LOGI(I2C_TAG, "Found %d pre-assigned SUBs", assigned_count);
    
    // Now scan the unassigned address range
    ESP_LOGI(I2C_TAG, "Now scanning for unassigned SUBs in range 0x%02X - 0x%02X", 
             I2C_SLAVE_ADDR_MIN, I2C_SLAVE_ADDR_MAX);
    scanned_count = 0;
    
    for (uint8_t addr = I2C_SLAVE_ADDR_MIN; addr <= I2C_SLAVE_ADDR_MAX; addr++) {
        scanned_count++;
        
        // Only log progress occasionally to reduce noise
        if (scanned_count % 16 == 0) {
            ESP_LOGI(I2C_TAG, "Scanning progress: %d/%d addresses...", 
                    scanned_count, I2C_SLAVE_ADDR_MAX - I2C_SLAVE_ADDR_MIN + 1);
        }

        // Skip if we've already found the maximum number of devices
        if (device_count >= MAX_SUB_NODES) {
            ESP_LOGI(I2C_TAG, "Maximum number of devices (%d) found, stopping scan", MAX_SUB_NODES);
            break;
        }

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
            continue;
        }

        // Try to write 0 bytes to detect if a device is present
        uint8_t dummy_data[1] = {0};
        ret = i2c_master_transmit(temp_dev_handle, dummy_data, 1, 10);
        
        if (ret == ESP_OK) {
            ESP_LOGI(I2C_TAG, "Device found at address 0x%02X", addr);
            
            // Add this device to our list
            sub_nodes[device_count].address = addr;
            sub_nodes[device_count].status = SUB_NODE_CONNECTED;
            sub_nodes[device_count].last_seen = get_current_time_ms();

            // Create a persistent device handle for this sub node
            i2c_device_config_t node_cfg = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = addr,
                .scl_speed_hz = 100000,
            };

            i2c_master_dev_handle_t node_handle;
            ret = i2c_master_bus_add_device(bus_handle, &node_cfg, &node_handle);
            if (ret != ESP_OK) {
                ESP_LOGE(I2C_TAG, "Failed to add device handle for sub node at address 0x%02X", addr);
                continue;
            }

            sub_nodes[device_count].handle = node_handle;
            
            // Try to read the identifier
            if (i2c_read_slave_identifier(device_count)) {
                ESP_LOGI(I2C_TAG, "Sub node %d: address=0x%02X, identifier=0x%02X", 
                        device_count, sub_nodes[device_count].address, sub_nodes[device_count].identifier);
            } else {
                ESP_LOGW(I2C_TAG, "Failed to read identifier for sub node at address 0x%02X", addr);
            }
            
            device_count++;
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

    // Count how many devices were found in the unassigned range
    unassigned_count = device_count - assigned_count;
    
    ESP_LOGI(I2C_TAG, "Scan complete: Found %d total I2C slave devices (%d pre-assigned, %d unassigned)", 
             device_count, assigned_count, unassigned_count);
    return true;
}

bool i2c_read_slave_identifier(int node_index) {
    if (node_index < 0 || node_index >= MAX_SUB_NODES) {
        ESP_LOGE(I2C_TAG, "Invalid node index: %d", node_index);
        return false;
    }

    if (sub_nodes[node_index].status == SUB_NODE_DISCONNECTED) {
        ESP_LOGE(I2C_TAG, "Node %d is not connected", node_index);
        return false;
    }

    ESP_LOGI(I2C_TAG, "Reading identifier from slave at address 0x%02X", sub_nodes[node_index].address);

    // Request the identifier from the slave
    uint8_t req_data[I2C_DATA_LEN] = {0};
    req_data[0] = msg_req_identifier;  // Message type
    req_data[1] = node_index;          // Device index
    req_data[2] = 0;                   // Data length

    // Send the request
    esp_err_t ret = i2c_master_transmit(sub_nodes[node_index].handle, req_data, I2C_DATA_LEN, 100);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "Error sending identifier request: %s", esp_err_to_name(ret));
        return false;
    }

    // Delay to give slave time to process the request
    vTaskDelay(pdMS_TO_TICKS(50));

    // Receive the response
    uint8_t res_data[I2C_DATA_LEN] = {0};
    ret = i2c_master_receive(sub_nodes[node_index].handle, res_data, I2C_DATA_LEN, 100);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "Error receiving identifier response: %s", esp_err_to_name(ret));
        return false;
    }

    // Check if the response is the expected type
    if (res_data[0] != msg_res_identifier) {
        ESP_LOGE(I2C_TAG, "Unexpected response type: %d", res_data[0]);
        return false;
    }

    // Extract the identifier from the response (single byte)
    uint8_t data_len = res_data[2];
    if (data_len >= 1) {
        sub_nodes[node_index].identifier = res_data[3];
        ESP_LOGI(I2C_TAG, "Sub node %d identifier: 0x%02X", node_index, sub_nodes[node_index].identifier);
        
        // Update last seen timestamp
        sub_nodes[node_index].last_seen = get_current_time_ms();
        return true;
    } else {
        ESP_LOGW(I2C_TAG, "Invalid identifier data length: %d", data_len);
        return false;
    }
}

int i2c_get_connected_node_count() {
    int count = 0;
    for (int i = 0; i < MAX_SUB_NODES; i++) {
        if (sub_nodes[i].status != SUB_NODE_DISCONNECTED) {
            count++;
        }
    }
    return count;
}

int i2c_get_first_node_index() {
    for (int i = 0; i < MAX_SUB_NODES; i++) {
        if (sub_nodes[i].status != SUB_NODE_DISCONNECTED) {
            return i;
        }
    }
    return -1;  // No connected nodes
}

bool i2c_write_to_node(int node_index, uint8_t *data_wr) {
    if (node_index < 0 || node_index >= MAX_SUB_NODES) {
        ESP_LOGE(I2C_TAG, "Invalid node index: %d", node_index);
        return false;
    }

    if (sub_nodes[node_index].status == SUB_NODE_DISCONNECTED) {
        ESP_LOGE(I2C_TAG, "Node %d is not connected", node_index);
        return false;
    }

    esp_err_t ret = i2c_master_transmit(sub_nodes[node_index].handle, data_wr, I2C_DATA_LEN, 100);

    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "Error sending data to node %d: %s", node_index, esp_err_to_name(ret));
        return false;
    }
    
    // Update last seen timestamp and status
    sub_nodes[node_index].last_seen = get_current_time_ms();
    sub_nodes[node_index].status = SUB_NODE_ACTIVE;
    
    ESP_LOGI(I2C_TAG, "Data sent successfully to node %d", node_index);
    return true;
}

bool i2c_is_node_connected(int node_index) {
    if (node_index < 0 || node_index >= MAX_SUB_NODES) {
        return false;
    }
    return (sub_nodes[node_index].status != SUB_NODE_DISCONNECTED);
}

const sub_node_t* i2c_get_node_info(int node_index) {
    if (node_index < 0 || node_index >= MAX_SUB_NODES) {
        return NULL;
    }
    return &sub_nodes[node_index];
}

// Legacy function for backward compatibility
bool i2c_write(uint8_t *data_wr) {
    int first_node = i2c_get_first_node_index();
    if (first_node < 0) {
        ESP_LOGE(I2C_TAG, "No connected nodes available");
        return false;
    }
    return i2c_write_to_node(first_node, data_wr);
}