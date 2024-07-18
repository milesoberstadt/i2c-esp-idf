#if !defined(__DEVICE_CONFIG_H__)
#define __DEVICE_CONFIG_H__

#include <stdbool.h>

#include "constants.h"
#include "data.h"
#include "types.h"
#include "uuid128.h"

#define DEVICE_CONFIG_TAG "DEVICE_CONFIG"

#define DEVICE_TYPE_COUNT 1

typedef enum device_type_t {
    UNKNOWN_DEVICE = -1,
    DEVICE_M_NODE = 0,
    DEVICE_A_NODE = 1,
    DEVICE_SLIPPER = 2,
} device_type_t;

bool init_device_config();

device_type_config_t get_device_config(device_type_t type);

device_type_t get_device_type_from_uuid(uint8_t *service_uuid);

#endif // __DEVICE_CONFIG_H__
