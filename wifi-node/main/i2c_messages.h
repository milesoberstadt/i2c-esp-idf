#if !defined(__I2C_MESSAGES_H__)
#define __I2C_MESSAGES_H__

#include <stdio.h>

#include "esp_log.h"
#include "esp_err.h"

#include "types.h"
#include "utils.h"
#include "constants.h"
#include "display.h"
#include "devices.h"

#include "i2c_slave.h"

#define I2C_MESSAGES "I2C_Messages"

void process_message(uint8_t* data, size_t length);

void i2c_send_message(message_t msg, size_t dev_idx);

void i2c_send_message_data(message_t msg, size_t dev_idx, uint8_t *data, size_t data_len);

#endif // __I2C_MESSAGES_H__
