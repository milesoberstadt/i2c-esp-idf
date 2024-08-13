#if !defined(__TYPES_H__)
#define __TYPES_H__

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

typedef enum device_state_t {
    dev_state_error = -1,
    dev_state_disconnected = 0,
    dev_state_connected = 1,
    dev_state_pairing = 2,
    dev_state_disconnecting = 3,
    dev_state_connecting = 4,
} device_state_t;

typedef enum device_type_t {
    UNKNOWN_DEVICE = -1,
    DEVICE_M_NODE = 0,
    DEVICE_A_NODE = 1,
    DEVICE_SLEEPER = 2,
} device_type_t;

typedef enum message_t {
    msg_err = 0,
    msg_init_start = 1,
    msg_init_end = 2,
    msg_dev_selected = 3,
    msg_dev_type = 10,
    msg_dev_state = 11,
    msg_dev_data = 12,
    msg_dev_error = 14,
    msg_dev_battery_level = 15,
    msg_screen_toggle = 50,
} message_t;

typedef struct device_t {
    device_type_t type;
    device_state_t state;
    uint8_t* value;
    uint8_t value_size;
} device_t;


#endif // __TYPES_H__
