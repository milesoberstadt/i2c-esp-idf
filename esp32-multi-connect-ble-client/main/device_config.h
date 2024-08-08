#if !defined(__DEVICE_CONFIG_H__)
#define __DEVICE_CONFIG_H__

#include <stdbool.h>

#include "constants.h"
#include "data.h"
#include "types.h"
#include "uuid128.h"

#define DEVICE_CONFIG_TAG "DEVICE_CONFIG"

/* --- Define your devices config here --- */

// Number of existing device types (without UNKNOWN_DEVICE)
#define DEVICE_TYPE_COUNT 2

// Give a unique ID to each device type
typedef enum device_type_t {
    UNKNOWN_DEVICE = -1,
    DEVICE_M_NODE = 0,
    DEVICE_A_NODE = 1,
    DEVICE_SLEEPER = 2,
} device_type_t;

/* M-node */

static const esp_bt_uuid_t UUID_M_NODE = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x14, 0x12, 0x8a, 0x76, 0x04, 0xd1, 0x6c, 0x4f, 0x7e, 0x53, 0xf2, 0xe8, 0x00, 0x00, 0xb1, 0x19 }
};

static const esp_bt_uuid_t UUID_GYRO_CHAR = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x2a, 0x5c, 0x76, 0xa3, 0xca, 0x25, 0x11, 0xb0, 0x25, 0x43, 0x9b, 0xaa, 0xd5, 0x1e, 0xd3, 0x31 }
};

static const esp_bt_uuid_t UUID_ACCEL_CHAR = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x4f, 0xff, 0x69, 0xed, 0xd2, 0xec, 0xb3, 0xa5, 0x30, 0x45, 0x7b, 0x0c, 0xbe, 0xdf, 0xd6, 0xbc }
};

static esp_bt_uuid_t M_NODE_CHAR_UUIDS[] = { UUID_GYRO_CHAR, UUID_ACCEL_CHAR };

/* A-node */

static const esp_bt_uuid_t UUID_A_NODE = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x54, 0x96, 0x1d, 0xff, 0xaf, 0x86, 0xab, 0xbb, 0x32, 0x41, 0x16, 0x36, 0x64, 0x19, 0x7e, 0x0c }
};

static const esp_bt_uuid_t UUID_WIND_SPEED_CHAR = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x1e, 0x7e, 0x30, 0x77, 0xe9, 0xb3, 0x63, 0xbd, 0x6f, 0x40, 0x2c, 0x70, 0x3b, 0x8e, 0x23, 0xe2 }
};

static const esp_bt_uuid_t UUID_WIND_DIRECTION_CHAR = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x0b, 0x4f, 0x24, 0x42, 0xf8, 0x06, 0x8b, 0xa0, 0xd9, 0x41, 0xf4, 0xce, 0x3a, 0xad, 0xf9, 0xfb }
};

static esp_bt_uuid_t A_NODE_CHAR_UUIDS[] = { UUID_WIND_SPEED_CHAR, UUID_WIND_DIRECTION_CHAR };

/* Device configurations */

static const device_type_config_t M_NODE_CONFIG = {
        .service_uuid = UUID_M_NODE,
        .char_uuids = M_NODE_CHAR_UUIDS,
        .char_count = 2,
        .data_callback = m_node_cb,
    };

static const device_type_config_t A_NODE_CONFIG = {
        .service_uuid = UUID_A_NODE,
        .char_uuids = A_NODE_CHAR_UUIDS,
        .char_count = 2,
        .data_callback = a_node_cb,
    };

static const device_type_config_t DEVICE_CONFIGS[] = {
    [DEVICE_M_NODE] = M_NODE_CONFIG,
    [DEVICE_A_NODE] = A_NODE_CONFIG,
};

/* --- Function definitions --- */

device_type_config_t get_device_config(device_type_t type);

device_type_t get_device_type_from_uuid(uint8_t *service_uuid);

#endif // __DEVICE_CONFIG_H__
