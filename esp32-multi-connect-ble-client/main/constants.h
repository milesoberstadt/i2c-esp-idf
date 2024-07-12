#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include "esp_bt_defs.h"
#include "esp_gatt_defs.h"

#define PROFILE_NUM 3
#define INVALID_HANDLE   0

#define REMOTE_NOTIFY_CHAR_UUID    0xFF01

#define USE_LED 1
#define LED_PIN GPIO_NUM_5

#define BUTTON_PIN GPIO_NUM_15

#define DEBOUNCE_TIME_MS 50

// 19b10000-e8f2-537e-4f6c-d104768a1214
static esp_bt_uuid_t remote_service_uuid = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {
        0x19, 0xb1, 0x00, 0x00, 
        0xe8, 0xf2, 
        0x53, 0x7e, 
        0x4f, 0x6c, 
        0xd1, 0x04, 0x76, 0x8a, 0x12, 0x14}},
};

// 31d31ed5-aa9b-4325-b011-25caa3765c2a
static esp_bt_uuid_t remote_gyro_uuid = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {
        0x31, 0xd3, 0x1e, 0xd5, 
        0xaa, 0x9b, 
        0x43, 0x25, 
        0xb0, 0x11, 
        0x25, 0xca, 0xa3, 0x76, 0x5c, 0x2a}},
};

// bcd6dfbe-0c7b-4530-a5b3-ecd2ed69ff4f
static esp_bt_uuid_t remote_accel_uuid = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {
        0xbc, 0xd6, 0xdf, 0xbe, 
        0x0c, 0x7b, 
        0x45, 0x30, 
        0xa5, 0xb3, 
        0xec, 0xd2, 0xed, 0x69, 0xff, 0x4f
    }},
};

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

#endif