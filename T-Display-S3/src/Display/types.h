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
    error = -1,
    disconnected = 0,
    connected = 1,
    pairing = 2,
    disconnecting = 3,
    connecting = 4,
} device_state_t;

typedef enum message_t {
    error_message = 0,
    pairing_start_message = 1,
    pairing_stop_message = 2,
    connected_message = 3,
    disconnected_message = 4,
    disconnecting_message = 5,
    connecting_message = 6,
    data_message = 7,
} message_t;

typedef struct device_t {
    device_type_t type;
    device_state_t state;
    uint8_t* value;
    uint8_t value_size;
} device_t;

#endif // __TYPES__
