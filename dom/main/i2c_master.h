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

// Slave device information
extern uint8_t discovered_slave_addr;  // The actual address of the discovered slave
extern uint16_t slave_identifier;      // The identifier of the discovered slave

bool i2c_init();
bool i2c_write(uint8_t *data_wr);
bool i2c_scan_for_slave();
bool i2c_read_slave_identifier();

#endif // __I2C_MASTER_H__