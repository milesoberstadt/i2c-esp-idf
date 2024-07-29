#if !defined(__I2C_MASTER_H__)
#define __I2C_MASTER_H__

#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "constants.h"
#include <stdbool.h>

#define I2C_TAG "I2C_Master"

bool i2c_master_init();
bool i2c_master_write_slave(uint8_t *data_wr);
bool i2c_master_read_slave();

#endif // __I2C_MASTER_H__