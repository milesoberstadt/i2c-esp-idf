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

#define i2c_MESSAGES "I2C_Messages"

void process_message(uint8_t* data, size_t length);

#endif // __I2C_MESSAGES_H__
