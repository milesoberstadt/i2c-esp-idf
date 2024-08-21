#if !defined(__DEVICE_CONFIG_H__)
#define __DEVICE_CONFIG_H__

#include <stdbool.h>

#include "esp_log.h"

#include "constants.h"
#include "types.h"
#include "uuid128.h"

#define DEVICE_CONFIG_TAG "DEVICE_CONFIG"

/* --- Define your devices config here --- */

// Give a unique ID to each device type
typedef enum device_type_t {
    UNKNOWN_DEVICE = -1,
    DEVICE_M_NODE = 0,
    DEVICE_A_NODE = 1,
    DEVICE_SLEEPER = 2,
} device_type_t;


// Number of existing device types (excluding UNKNOWN_DEVICE)
#define DEVICE_TYPE_COUNT 3


/* --- Common --- */

// "bbd16cf5-9234-43eb-93b5-679e4142f689"
static const esp_bt_uuid_t UUID_BATTERY_SERVICE = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x89, 0xf6, 0x42, 0x41, 0x9e, 0x67, 0xb5, 0x93, 0xeb, 0x43, 0x34, 0x92, 0xf5, 0x6c, 0xd1, 0xbb }
};

// "12fb1e82-a141-429a-86c1-6be3f0c561af"
static const esp_bt_uuid_t UUID_BATTERY_CHARACTERISTIC = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0xaf, 0x61, 0xc5, 0xf0, 0xe3, 0x6b, 0xc1, 0x86, 0x9a, 0x42, 0x41, 0xa1, 0x82, 0x1e, 0xfb, 0x12 }
};


/* --- M-node --- */

//"19b10000-e8f2-537e-4f6c-d104768a1214"
static const esp_bt_uuid_t UUID_M_NODE = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x14, 0x12, 0x8a, 0x76, 0x04, 0xd1, 0x6c, 0x4f, 0x7e, 0x53, 0xf2, 0xe8, 0x00, 0x00, 0xb1, 0x19 }
};

// "31d31ed5-aa9b-4325-b011-25caa3765c2a"
static const esp_bt_uuid_t UUID_GYRO_CHAR = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x2a, 0x5c, 0x76, 0xa3, 0xca, 0x25, 0x11, 0xb0, 0x25, 0x43, 0x9b, 0xaa, 0xd5, 0x1e, 0xd3, 0x31 }
};

//"bcd6dfbe-0c7b-4530-a5b3-ecd2ed69ff4f"
static const esp_bt_uuid_t UUID_ACCEL_CHAR = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x4f, 0xff, 0x69, 0xed, 0xd2, 0xec, 0xb3, 0xa5, 0x30, 0x45, 0x7b, 0x0c, 0xbe, 0xdf, 0xd6, 0xbc }
};

static esp_bt_uuid_t M_NODE_CHAR_UUIDS[] = { UUID_GYRO_CHAR, UUID_ACCEL_CHAR };


/* --- A-node --- */

//"0c7e1964-3616-4132-bbab-86afff1d9654"
static const esp_bt_uuid_t UUID_A_NODE = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x54, 0x96, 0x1d, 0xff, 0xaf, 0x86, 0xab, 0xbb, 0x32, 0x41, 0x16, 0x36, 0x64, 0x19, 0x7e, 0x0c }
};

//"e2238e3b-702c-406f-bd63-b3e977307e1e"
static const esp_bt_uuid_t UUID_WIND_SPEED_CHAR = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x1e, 0x7e, 0x30, 0x77, 0xe9, 0xb3, 0x63, 0xbd, 0x6f, 0x40, 0x2c, 0x70, 0x3b, 0x8e, 0x23, 0xe2 }
};

//"fbf9ad3a-cef4-41d9-a08b-06f8424a1fb0"
static const esp_bt_uuid_t UUID_WIND_DIRECTION_CHAR = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0xb0, 0x1f, 0x4a, 0x42, 0xf8, 0x06, 0x8b, 0xa0, 0xd9, 0x41, 0xf4, 0xce, 0x3a, 0xad, 0xf9, 0xfb }
};

static esp_bt_uuid_t A_NODE_CHAR_UUIDS[] = { UUID_WIND_SPEED_CHAR, UUID_WIND_DIRECTION_CHAR };


/* --- Sleeper --- */

//"63e4eb54-b0bc-4374-8d2a-5f08f951230a"
static const esp_bt_uuid_t UUID_SLEEPER_NODE = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x0a, 0x23, 0x51, 0xf9, 0x08, 0x5f, 0x2a, 0x8d, 0x74, 0x43, 0xbc, 0xb0, 0x54, 0xeb, 0xe4, 0x63 }
};

//"2c41ce1f-acd3-4088-8394-b21a88e88142"
static const esp_bt_uuid_t UUID_SLEEP_CHAR = {
    .len = ESP_UUID_LEN_128,
    .uuid.uuid128 = { 0x42, 0x81, 0xe8, 0x88, 0x1a, 0xb2, 0x94, 0x83, 0x88, 0x40, 0xd3, 0xac, 0x1f, 0xce, 0x41, 0x2c }
};

static esp_bt_uuid_t SLEEPER_CHAR_UUIDS[] = { UUID_SLEEP_CHAR };


/* --- Device configurations --- */

static const device_type_config_t M_NODE_CONFIG = {
        .service_uuid = UUID_M_NODE,
        .char_uuids = M_NODE_CHAR_UUIDS,
        .char_count = 2,
    };

static const device_type_config_t A_NODE_CONFIG = {
        .service_uuid = UUID_A_NODE,
        .char_uuids = A_NODE_CHAR_UUIDS,
        .char_count = 2,
    };

static const device_type_config_t SLEEPER_CONFIG = {
        .service_uuid = UUID_SLEEPER_NODE,
        .char_uuids = SLEEPER_CHAR_UUIDS,
        .char_count = 1,
    };

static const device_type_config_t DEVICE_CONFIGS[] = {
    [DEVICE_M_NODE] = M_NODE_CONFIG,
    [DEVICE_A_NODE] = A_NODE_CONFIG,
    [DEVICE_SLEEPER] = SLEEPER_CONFIG,
};


typedef struct device_t {
    device_type_t type;
    device_state_t state;
    uint8_t battery_level;
} device_t;

/* --- Function definitions --- */

device_type_config_t get_device_config(device_type_t type);

device_type_t get_device_type_from_uuid(uint8_t *service_uuid);

#endif // __DEVICE_CONFIG_H__
