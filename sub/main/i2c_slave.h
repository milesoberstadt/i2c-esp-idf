#ifndef __I2C_SLAVE_H__
#define __I2C_SLAVE_H__

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/i2c_slave.h"
#include "driver/i2c_types.h"

#include "constants.h"

#define I2C_SLAVE_TAG "I2C_Slave"

void i2c_slave_init();
void i2c_start_receive();
bool i2c_slave_send(uint8_t* data, size_t length);

#endif // __I2C_SLAVE_H__