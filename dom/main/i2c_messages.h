#if !defined(__I2C_MESSAGES__)
#define __I2C_MESSAGES__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_err.h"

#include "i2c_master.h"
#include "types.h"
#include "constants.h"

#define I2C_MSG_TAG "I2C_MSG"

// For legacy support (sends to first available node)
void i2c_send_message(message_t msg, uint8_t dev_idx);
void i2c_send_message_data(message_t msg, uint8_t dev_idx, uint8_t *data, size_t len);

// Send to specific node by index
void i2c_send_message_to_node(int node_index, message_t msg, uint8_t dev_idx);
void i2c_send_message_data_to_node(int node_index, message_t msg, uint8_t dev_idx, uint8_t *data, size_t len);

// Send to all connected nodes (broadcast)
void i2c_broadcast_message(message_t msg, uint8_t dev_idx);
void i2c_broadcast_message_data(message_t msg, uint8_t dev_idx, uint8_t *data, size_t len);

// Set WiFi channel for a specific sub node
void i2c_set_sub_wifi_channel(int node_index, uint8_t channel);

// Process received messages
void process_message(uint8_t* data, size_t length);

#endif // __I2C_MESSAGES__