#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_slave.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "types.h"

// I2C Slave configuration
#define I2C_SLAVE_SCL_IO           5   // GPIO pin for SCL
#define I2C_SLAVE_SDA_IO           4   // GPIO pin for SDA
#define I2C_SLAVE_PORT             I2C_NUM_0
#define I2C_SLAVE_RX_BUF_LEN       256
#define I2C_SLAVE_TX_BUF_LEN       256

// Fixed I2C address for the SUB node
#define I2C_FIXED_SLAVE_ADDR       0x42

// I2C address ranges (for compatibility with DOM node)
#define I2C_ADDR_RANDOM_MIN        0x20    // Start of random address range
#define I2C_ADDR_RANDOM_MAX        0x27    // End of random address range
#define I2C_ADDR_RESERVED_MIN      0x40    // Start of reserved address range
#define I2C_ADDR_RESERVED_MAX      0x47    // End of reserved address range

// I2C message header and data sizes
#define I2C_HEADER_LEN             3     // msg_type (1) + sub_id (1) + data_len (1)
#define I2C_MESSAGE_DATA_LEN       (I2C_DATA_LEN - I2C_HEADER_LEN)

#define I2C_SLAVE_TAG              "i2c_slave"

// Initialize I2C slave with fixed address
esp_err_t i2c_slave_init(void);

// Start I2C slave operation
esp_err_t i2c_slave_begin(void);

// Stop I2C slave operation
esp_err_t i2c_slave_stop(void);

// Get the current SUB configuration
sub_config_t* i2c_slave_get_config(void);

// Check if we've been verified by the DOM
bool i2c_slave_is_verified(void);

// Reset the verification status
void i2c_slave_reset_verification(void);

// Prepare a response message
void i2c_slave_prepare_response(uint8_t msg_type, const uint8_t *data, uint8_t data_len);

#endif // I2C_SLAVE_H