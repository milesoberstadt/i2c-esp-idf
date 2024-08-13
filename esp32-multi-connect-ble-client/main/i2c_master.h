#if !defined(__I2C_MASTER_H__)
#define __I2C_MASTER_H__

#include <stdio.h>
#include <stdbool.h>

#include "driver/i2c_master.h"

#include "esp_log.h"

#include "constants.h"

#define I2C_TAG "I2C_Master"

bool i2c_master_init();
bool i2c_master_write_slave(uint8_t *data_wr, size_t len, uint8_t addr);
bool i2c_master_read_slave(uint8_t addr, uint8_t *data_received);

#endif // __I2C_MASTER_H__