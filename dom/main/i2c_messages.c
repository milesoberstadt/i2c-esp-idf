#include "i2c_messages.h"

#define HEADER_LEN 4

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