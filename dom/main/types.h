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
} message_t;

#endif // __TYPES_H__