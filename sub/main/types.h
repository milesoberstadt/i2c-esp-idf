#if !defined(__TYPES_H__)
#define __TYPES_H__

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

typedef enum message_t {
    msg_err = 0x00,
    msg_init_start = 0x01,
    msg_init_end = 0x02,
    msg_data = 0x10,
    msg_req_data = 0x20,
    msg_res_data = 0x21,
    msg_req_identifier = 0x30,
    msg_res_identifier = 0x31,
    msg_set_i2c_address = 0x38,   // New message for I2C address reassignment
    msg_set_wifi_channel = 0x40,
} message_t;

#endif // __TYPES_H__