#if !defined(__I2C_MASTER_H__)
#define __I2C_MASTER_H__

#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "constants.h"
#include <stdbool.h>

static const char *TAG = "I2C_Master";
uint8_t data_received;
esp_err_t ret;

bool i2c_master_init();
bool i2c_master_write_slave(uint8_t *data_wr, size_t length);
bool i2c_master_read_slave();

#endif // __I2C_MASTER_H__