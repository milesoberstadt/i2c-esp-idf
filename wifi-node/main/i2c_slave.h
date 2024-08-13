#ifndef __I2C_SLAVE_H__
#define __I2C_SLAVE_H__

#include <stdio.h>
#include "driver/i2c_slave.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"

#include "esp_attr.h"

#include "esp_err.h"
#include "esp_log.h"
    
#include "constants.h"
#include "i2c_messages.h"

#define i2c_SLAVE "I2C_Slave"

void init_i2c_slave();
void i2c_receive();
// void i2c_slave_send(uint8_t data_to_send);

#endif // __I2C_SLAVE_H__