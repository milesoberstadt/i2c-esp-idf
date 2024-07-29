#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include "esp_bt_defs.h"
#include "esp_gatt_defs.h"

/* Maximum connected devices count */
#define MAX_DEVICES 4

/* BLE configuration */
#define PAIRING_DURATION 30 // seconds

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

/* I2C configuration */
#define I2C_MASTER_SCL_IO           9
#define I2C_MASTER_SDA_IO           8
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_SLAVE_ADDR              0x28

#define I2C_BUFFER_SIZE 20

/* Do no touch */

#define INVALID_HANDLE   0
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

#endif