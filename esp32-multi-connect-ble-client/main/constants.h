#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include "esp_bt_defs.h"
#include "esp_gatt_defs.h"

/* Maximum connected devices count */
#define MAX_DEVICES 4

// led pins start from here
#define LED_PIN GPIO_NUM_4
// to LED_PIN + MAX_DEVICES

// enable led state logging
#define LOG_LED 1

/* Buttons */
#define PAIR_BUTTON_PIN GPIO_NUM_1
#define SELECT_BUTTON_PIN GPIO_NUM_2
#define BUTTON_DEBOUNCE_TIME 50 // ms

/* Storage*/
#define PREFERENCES_PARTITION "MCBC"


/* Do no touch */

#define INVALID_HANDLE   0
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

#endif