#if !defined(__I2C_MESSAGES_H__)
#define __I2C_MESSAGES_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"

#include "types.h"
#include "constants.h"
#include "i2c_slave.h"

#define I2C_MESSAGES_TAG "I2C_Messages"

void process_message(uint8_t* data, size_t length);
void i2c_send_message(message_t msg, uint8_t dev_idx);
void i2c_send_message_data(message_t msg, uint8_t dev_idx, uint8_t *data, size_t data_len);

#endif // __I2C_MESSAGES_H__