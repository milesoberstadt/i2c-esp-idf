#ifndef __I2C_SLAVE_H__
#define __I2C_SLAVE_H__

#include <stdio.h>
#include "driver/i2c_slave.h"
#include "driver/i2c_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
    
#include "constants.h"
#include "i2c_messages.h"

#define i2c_SLAVE "I2C_Slave"

void i2c_slave_init();
void i2c_receive(uint8_t *data_received);

#endif // __I2C_SLAVE_H__