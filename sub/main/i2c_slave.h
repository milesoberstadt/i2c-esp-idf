#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "types.h"

// Random I2C address range (for initial discovery)
#define I2C_ADDR_RANDOM_MIN 20
#define I2C_ADDR_RANDOM_MAX 127

// Reserved I2C address range (for DOM to assign to SUBs)
#define I2C_ADDR_RESERVED_MIN 1
#define I2C_ADDR_RESERVED_MAX 19

// Initialize I2C slave with random address
esp_err_t i2c_slave_init(void);

// Change I2C slave address
esp_err_t i2c_slave_set_address(uint8_t new_addr);

// Start I2C slave operation
esp_err_t i2c_slave_start(void);

// Stop I2C slave operation
esp_err_t i2c_slave_stop(void);

// Get the current SUB configuration
sub_config_t* i2c_slave_get_config(void);

// Check if we've been verified by the DOM
bool i2c_slave_is_verified(void);

// Reset the verification status
void i2c_slave_reset_verification(void);

#endif // I2C_SLAVE_H