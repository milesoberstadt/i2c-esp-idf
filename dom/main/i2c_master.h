#if !defined(__I2C_MASTER_H__)
#define __I2C_MASTER_H__

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "driver/i2c_types.h"

#include "constants.h"

#define I2C_TAG "I2C_Master"

// Min and max I2C addresses to scan for the slave
#define I2C_SLAVE_ADDR_MIN   0x14  // 20 decimal
#define I2C_SLAVE_ADDR_MAX   0x78  // 120 decimal

// Maximum number of sub nodes that can be connected
#define MAX_SUB_NODES        11

// Sub node connection status
typedef enum {
    SUB_NODE_DISCONNECTED = 0,   // No device connected
    SUB_NODE_CONNECTED = 1,      // Device found and connected
    SUB_NODE_ACTIVE = 2          // Device is actively communicating
} sub_node_status_t;

// Structure to hold information about a sub node
typedef struct {
    uint8_t address;             // I2C address
    uint8_t identifier;          // Device identifier (single byte)
    i2c_master_dev_handle_t handle;  // Device handle
    sub_node_status_t status;    // Connection status
    uint32_t last_seen;          // Timestamp of last communication
} sub_node_t;

// Initialize the I2C master and device pool
bool i2c_init();

// Scan for sub nodes in the specified address range
bool i2c_scan_for_slaves();

// Read the identifier from a detected slave
bool i2c_read_slave_identifier(int node_index);

// Get the number of connected sub nodes
int i2c_get_connected_node_count();

// Get the index of the first connected node (for simple applications that only need one node)
int i2c_get_first_node_index();

// Send data to a specific sub node by index
bool i2c_write_to_node(int node_index, uint8_t *data_wr);

// Check if a node at a specific index is connected
bool i2c_is_node_connected(int node_index);

// Get node information for a specific index
const sub_node_t* i2c_get_node_info(int node_index);

#endif // __I2C_MASTER_H__