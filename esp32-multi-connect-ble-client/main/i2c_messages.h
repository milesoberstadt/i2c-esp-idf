#if !defined(__I2C_MESSAGES__)
#define __I2C_MESSAGES__

#include <stdio.h>

#include "i2c_master.h"
#include "types.h"
#include "device_config.h"
#include "ui.h"

#define I2C_MSG_TAG "I2C_MSG"

void i2c_send_message(message_t msg, size_t dev_idx);

void i2c_send_message_data(message_t msg, size_t dev_idx, uint8_t *data, size_t len);

#endif // __I2C_MESSAGES__
