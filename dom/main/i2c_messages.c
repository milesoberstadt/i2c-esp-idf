#include "i2c_messages.h"

#define HEADER_LEN 4

// Reference to the sub_nodes array defined in i2c_master.c
extern sub_node_t sub_nodes[MAX_SUB_NODES];

// Helper function to prepare message data
static uint8_t* prepare_message_data(message_t msg, uint8_t dev_idx, uint8_t *data, size_t data_len) {
    if (data_len > I2C_DATA_LEN - HEADER_LEN) {
        ESP_LOGE(I2C_MSG_TAG, "Data length exceeds maximum allowed length");
        return NULL;
    }

    uint8_t *msg_data = (uint8_t *)malloc(I2C_DATA_LEN);
    if (msg_data == NULL) {
        ESP_LOGE(I2C_MSG_TAG, "Failed to allocate memory for message data");
        return NULL;
    }

    msg_data[0] = msg;
    msg_data[1] = dev_idx;
    msg_data[2] = data_len;
    
    // The 4th byte is the identity byte, which is only set when sending to a node with a known identifier
    // For broadcasts or nodes without identity yet, it will be 0
    msg_data[3] = 0; // Default to 0 for now, will be set in the specific sending functions

    if (data != NULL && data_len > 0) {
        memcpy(msg_data + HEADER_LEN, data, data_len);
    }

    for (size_t i = HEADER_LEN + data_len; i < I2C_DATA_LEN; i++) {
        msg_data[i] = 0xFF;
    }

    return msg_data;
}

// Legacy support - sends to first available node
void i2c_send_message(message_t msg, uint8_t dev_idx) {
    i2c_send_message_data(msg, dev_idx, NULL, 0);
}

void i2c_send_message_data(message_t msg, uint8_t dev_idx, uint8_t *data, size_t data_len) {
    int first_node = i2c_get_first_node_index();
    if (first_node < 0) {
        ESP_LOGE(I2C_MSG_TAG, "No connected nodes available");
        return;
    }
    
    i2c_send_message_data_to_node(first_node, msg, dev_idx, data, data_len);
}

// Send to specific node by index
void i2c_send_message_to_node(int node_index, message_t msg, uint8_t dev_idx) {
    i2c_send_message_data_to_node(node_index, msg, dev_idx, NULL, 0);
}

void i2c_send_message_data_to_node(int node_index, message_t msg, uint8_t dev_idx, uint8_t *data, size_t data_len) {
    if (!i2c_is_node_connected(node_index)) {
        ESP_LOGE(I2C_MSG_TAG, "Node %d is not connected", node_index);
        return;
    }

    uint8_t *msg_data = prepare_message_data(msg, dev_idx, data, data_len);
    if (msg_data == NULL) {
        return;
    }

    // Get the node info to access the identifier and add it to the message header
    const sub_node_t* node_info = i2c_get_node_info(node_index);
    if (node_info != NULL && node_info->identifier != 0) {
        // Set the identity byte in the header
        msg_data[3] = node_info->identifier;
        ESP_LOGD(I2C_MSG_TAG, "Adding node identifier 0x%02X to message header", node_info->identifier);
    } else {
        // For messages to nodes without an identifier yet (like during initial setup)
        msg_data[3] = 0;
    }

    if (i2c_write_to_node(node_index, msg_data)) {
        ESP_LOGI(I2C_MSG_TAG, "Message sent to node %d: type=%d, with identifier=0x%02X", 
                node_index, msg, msg_data[3]);
        esp_log_buffer_hex(I2C_MSG_TAG, msg_data, I2C_DATA_LEN);
    } else {
        ESP_LOGE(I2C_MSG_TAG, "Failed to send message to node %d", node_index);
    }

    free(msg_data);
}

// Send to all connected nodes (broadcast)
void i2c_broadcast_message(message_t msg, uint8_t dev_idx) {
    i2c_broadcast_message_data(msg, dev_idx, NULL, 0);
}

void i2c_broadcast_message_data(message_t msg, uint8_t dev_idx, uint8_t *data, size_t data_len) {
    // For broadcast messages, we need to send individually to each node with its own identifier
    int success_count = 0;
    int total_count = 0;

    // Send to all connected nodes
    for (int i = 0; i < MAX_SUB_NODES; i++) {
        if (i2c_is_node_connected(i)) {
            total_count++;
            
            // Use the node-specific send function to ensure proper identifier is set
            i2c_send_message_data_to_node(i, msg, dev_idx, data, data_len);
            success_count++;
            ESP_LOGD(I2C_MSG_TAG, "Broadcast message sent to node %d", i);
        }
    }

    ESP_LOGI(I2C_MSG_TAG, "Broadcast message sent to %d/%d nodes: type=%d", 
            success_count, total_count, msg);
}

void i2c_set_sub_wifi_channel(int node_index, uint8_t channel) {
    if (node_index < 0 || node_index >= MAX_SUB_NODES) {
        ESP_LOGE(I2C_MSG_TAG, "Invalid node index: %d", node_index);
        return;
    }

    if (!i2c_is_node_connected(node_index)) {
        ESP_LOGE(I2C_MSG_TAG, "Node %d is not connected", node_index);
        return;
    }

    // Get the node info to access the identifier
    const sub_node_t* node_info = i2c_get_node_info(node_index);
    if (node_info == NULL) {
        ESP_LOGE(I2C_MSG_TAG, "Failed to get node info for node %d", node_index);
        return;
    }

    ESP_LOGI(I2C_MSG_TAG, "Setting WiFi channel %d for sub node %d (identifier: 0x%02X)", 
             channel, node_index, node_info->identifier);
    
    // Send only the channel data - the identifier will be included in the header
    uint8_t data[1] = {
        channel  // WiFi channel
    };
    
    i2c_send_message_data_to_node(node_index, msg_set_wifi_channel, 0, data, 1);
}

void i2c_set_sub_i2c_address(int node_index, uint8_t new_address) {
    if (node_index < 0 || node_index >= MAX_SUB_NODES) {
        ESP_LOGE(I2C_MSG_TAG, "Invalid node index: %d", node_index);
        return;
    }

    if (!i2c_is_node_connected(node_index)) {
        ESP_LOGE(I2C_MSG_TAG, "Node %d is not connected", node_index);
        return;
    }

    // Get the node info to access the identifier
    const sub_node_t* node_info = i2c_get_node_info(node_index);
    if (node_info == NULL) {
        ESP_LOGE(I2C_MSG_TAG, "Failed to get node info for node %d", node_index);
        return;
    }

    // Check if address is in valid range (0x08-0x77 for 7-bit addresses)
    if (new_address < 0x08 || new_address > 0x77) {
        ESP_LOGE(I2C_MSG_TAG, "Invalid I2C address: 0x%02X. Must be between 0x08 and 0x77", new_address);
        return;
    }

    ESP_LOGI(I2C_MSG_TAG, "Setting new I2C address 0x%02X for sub node %d (current addr: 0x%02X, identifier: 0x%02X)", 
             new_address, node_index, node_info->address, node_info->identifier);
    
    // Send the new I2C address
    uint8_t data[1] = {
        new_address  // New I2C address
    };
    
    i2c_send_message_data_to_node(node_index, msg_set_i2c_address, 0, data, 1);
    
    // We need to give the sub node time to process the address change and reinitialize
    // before we try to communicate with it again
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Note: After reassigning addresses, the DOM node will need to rescan for slaves
    // to update its local tracking and establish communication on the new addresses
}

void i2c_reassign_all_i2c_addresses() {
    ESP_LOGI(I2C_MSG_TAG, "Reassigning I2C addresses for unassigned nodes");
    
    // Count how many nodes are connected and determine which need assignment
    int connected_count = 0;
    int need_assignment_count = 0;
    int already_assigned_count = 0;
    
    for (int i = 0; i < MAX_SUB_NODES; i++) {
        if (i2c_is_node_connected(i)) {
            connected_count++;
            
            // Check if this node is already in the assigned address range
            if (sub_nodes[i].address >= I2C_ASSIGNED_ADDR_MIN && 
                sub_nodes[i].address <= I2C_ASSIGNED_ADDR_MAX) {
                // This node is already in the assigned range - skip it
                ESP_LOGI(I2C_MSG_TAG, "Node %d already has assigned address 0x%02X - skipping", 
                         i, sub_nodes[i].address);
                already_assigned_count++;
            } else {
                // This node needs assignment
                need_assignment_count++;
            }
        }
    }
    
    if (connected_count == 0) {
        ESP_LOGW(I2C_MSG_TAG, "No connected nodes found for I2C address reassignment");
        return;
    }
    
    if (need_assignment_count == 0) {
        ESP_LOGI(I2C_MSG_TAG, "All %d connected nodes already have assigned addresses - no reassignment needed", 
                 already_assigned_count);
        return;
    }
    
    ESP_LOGI(I2C_MSG_TAG, "Found %d connected nodes (%d already assigned, %d need assignment)",
             connected_count, already_assigned_count, need_assignment_count);
    
    // Keep track of which addresses are already in use
    bool address_used[256] = {false};
    
    // Mark addresses that are already in use
    for (int i = 0; i < MAX_SUB_NODES; i++) {
        if (i2c_is_node_connected(i) && 
            sub_nodes[i].address >= I2C_ASSIGNED_ADDR_MIN && 
            sub_nodes[i].address <= I2C_ASSIGNED_ADDR_MAX) {
            
            address_used[sub_nodes[i].address] = true;
            ESP_LOGD(I2C_MSG_TAG, "Address 0x%02X is already in use", sub_nodes[i].address);
        }
    }
    
    // Use the reserved range for verified subs (0x0A-0x1E)
    int assigned = 0;
    
    // For each node that needs assignment
    for (int i = 0; i < MAX_SUB_NODES; i++) {
        if (i2c_is_node_connected(i) && 
            (sub_nodes[i].address < I2C_ASSIGNED_ADDR_MIN || 
             sub_nodes[i].address > I2C_ASSIGNED_ADDR_MAX)) {
            
            // Find the next available address
            uint8_t new_address = 0;
            for (uint8_t addr = I2C_ASSIGNED_ADDR_MIN; addr <= I2C_ASSIGNED_ADDR_MAX; addr++) {
                if (!address_used[addr]) {
                    new_address = addr;
                    address_used[addr] = true; // Mark as used
                    break;
                }
            }
            
            // Make sure we found an available address
            if (new_address >= I2C_ASSIGNED_ADDR_MIN && new_address <= I2C_ASSIGNED_ADDR_MAX) {
                i2c_set_sub_i2c_address(i, new_address);
                ESP_LOGI(I2C_MSG_TAG, "Assigned I2C address 0x%02X to sub node %d", new_address, i);
                assigned++;
                
                // Add a small delay between assignments to avoid bus congestion
                vTaskDelay(pdMS_TO_TICKS(50));
            } else {
                ESP_LOGW(I2C_MSG_TAG, "Ran out of valid I2C addresses in reserved range, not reassigning address for node %d", i);
            }
        }
    }
    
    ESP_LOGI(I2C_MSG_TAG, "I2C address reassignment complete. Assigned %d/%d needed addresses", 
             assigned, need_assignment_count);
    
    // After reassignment, we should rescan the I2C bus to find the devices at their new addresses
    // This needs to be handled in the main flow
}

void process_message(uint8_t* data, size_t length) {
    if (data == NULL || length == 0) {
        ESP_LOGE(I2C_MSG_TAG, "Invalid data received");
        return;
    }

    esp_log_buffer_hex(I2C_MSG_TAG, data, length);

    uint8_t msg_type = data[0];
    uint8_t dev_idx = data[1];
    uint8_t msg_len = data[2];
    uint8_t identity = data[3];

    ESP_LOGI(I2C_MSG_TAG, "Message received - type: %d, device: %d, length: %d, identity: 0x%02X", 
             msg_type, dev_idx, msg_len, identity);

    // Handle message based on type
    switch (msg_type) {
        case msg_res_data:
            ESP_LOGI(I2C_MSG_TAG, "Data response received from sub node with identity 0x%02X", identity);
            if (msg_len > 0) {
                ESP_LOGI(I2C_MSG_TAG, "Received data: ");
                esp_log_buffer_hex(I2C_MSG_TAG, data + HEADER_LEN, msg_len);
            }
            break;
            
        case msg_res_identifier:
            // Special handling for identifier response - this may still use the old format
            // since it's received before we know the identifier
            ESP_LOGI(I2C_MSG_TAG, "Identifier response received");
            break;
        
        default:
            ESP_LOGI(I2C_MSG_TAG, "Unknown message type received: %d", msg_type);
            break;
    }
}