#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include "esp_bt_defs.h"
#include "esp_gatt_defs.h"

#define MAX_DEVICES 4
#define INVALID_HANDLE   0

#define REMOTE_NOTIFY_CHAR_UUID    0xFF01

// led pins start from here
#define LED_PIN GPIO_NUM_4
// to LED_PIN + MAX_DEVICES

#define LOG_LED 1

#define PAIR_BUTTON_PIN GPIO_NUM_1
#define SELECT_BUTTON_PIN GPIO_NUM_2

#define PREFERENCES_PARTITION "MCBC"

enum DeviceType {
    DEVICE_A_NODE,
    DEVICE_M_NODE,
    DEVICE_SLIPPER,
};

// 19b10000-e8f2-537e-4f6c-d104768a1214
static esp_bt_uuid_t remote_service_uuid = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {
        0x14, 0x12, 0x8a, 0x76, 
        0x04, 0xd1, 
        0x6c, 0x4f, 
        0x7e, 0x53, 
        0xf2, 0xe8, 
        0x00, 0x00, 
        0xb1, 0x19
    }},
};

// 31d31ed5-aa9b-4325-b011-25caa3765c2a
static esp_bt_uuid_t remote_gyro_uuid = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {
        0x2a, 0x5c, 0x76, 0xa3, 
        0xca, 0x25, 
        0x11, 0xb0, 
        0x25, 0x43, 
        0x9b, 0xaa, 
        0xd5, 0x1e, 
        0xd3, 0x31
    }},
};

// bcd6dfbe-0c7b-4530-a5b3-ecd2ed69ff4f
static esp_bt_uuid_t remote_accel_uuid = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {
        0x4f, 0xff, 0x69, 0xed, 
        0xd2, 0xec, 
        0xb3, 0xa5, 
        0x30, 0x45, 
        0x7b, 0x0c, 
        0xbe, 0xdf, 
        0xd6, 0xbc
    }},
};

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

#endif