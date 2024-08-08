#ifndef __I2C_SLAVE_H__
#define __I2C_SLAVE_H__

#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "constants.h"

#define i2c_TAG "I2C_Slave"

void i2c_slave_init();
uint8_t i2c_slave_receive();
void i2c_slave_send(uint8_t data_to_send);
#endif // __I2C_SLAVE_H__