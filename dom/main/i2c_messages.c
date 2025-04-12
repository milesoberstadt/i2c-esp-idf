#include "i2c_messages.h"

#define HEADER_LEN 3

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

    if (i2c_write_to_node(node_index, msg_data)) {
        ESP_LOGI(I2C_MSG_TAG, "Message sent to node %d: type=%d", node_index, msg);
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
    uint8_t *msg_data = prepare_message_data(msg, dev_idx, data, data_len);
    if (msg_data == NULL) {
        return;
    }

    int success_count = 0;
    int total_count = 0;

    // Send to all connected nodes
    for (int i = 0; i < MAX_SUB_NODES; i++) {
        if (i2c_is_node_connected(i)) {
            total_count++;
            if (i2c_write_to_node(i, msg_data)) {
                success_count++;
                ESP_LOGD(I2C_MSG_TAG, "Broadcast message sent to node %d", i);
            } else {
                ESP_LOGW(I2C_MSG_TAG, "Failed to send broadcast message to node %d", i);
            }
        }
    }

    ESP_LOGI(I2C_MSG_TAG, "Broadcast message sent to %d/%d nodes: type=%d", 
            success_count, total_count, msg);
    esp_log_buffer_hex(I2C_MSG_TAG, msg_data, I2C_DATA_LEN);

    free(msg_data);
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

    ESP_LOGI(I2C_MSG_TAG, "Setting WiFi channel %d for sub node %d", channel, node_index);
    
    uint8_t channel_data[1] = {channel};
    i2c_send_message_data_to_node(node_index, msg_set_wifi_channel, 0, channel_data, 1);
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

    ESP_LOGI(I2C_MSG_TAG, "Message received - type: %d, device: %d, length: %d", 
             msg_type, dev_idx, msg_len);

    // Handle message based on type
    switch (msg_type) {
        case msg_res_data:
            ESP_LOGI(I2C_MSG_TAG, "Data response received from sub node");
            if (msg_len > 0) {
                ESP_LOGI(I2C_MSG_TAG, "Received data: ");
                esp_log_buffer_hex(I2C_MSG_TAG, data + HEADER_LEN, msg_len);
            }
            break;
        
        default:
            ESP_LOGI(I2C_MSG_TAG, "Unknown message type received: %d", msg_type);
            break;
    }
}