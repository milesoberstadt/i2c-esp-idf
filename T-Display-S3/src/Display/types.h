#if !defined(__TYPES__)
#define __TYPES__

#include <stdint.h>

#define DEVICE_TYPE_COUNT 3
#define DEVICE_COUNT 6

// Give a unique ID to each device type
typedef enum device_type_t {
    UNKNOWN_DEVICE = -1,
    DEVICE_M_NODE = 0,
    DEVICE_A_NODE = 1,
    DEVICE_SLEEPER = 2,
} device_type_t;

typedef enum device_state_t {
    dev_state_error = -1,
    dev_state_disconnected = 0,
    dev_state_connected = 1,
    dev_state_pairing = 2,
    dev_state_disconnecting = 3,
    dev_state_connecting = 4,
} device_state_t;

typedef enum message_t {
    message_err = 0,
    message_init_start = 1,
    message_init_end = 2,
    message_dev_type = 10
    message_dev_state = 11,
    message_dev_data = 12,
    message_dev_selected = 13,
    message_dev_error = 14
} message_t;

#endif // __TYPES__
