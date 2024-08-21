#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

/* Maximum connected devices count */
#define MAX_DEVICES 8

/* BLE configuration */
#define PAIRING_DURATION 30 // seconds

// led pins start from here
#define LED_PIN GPIO_NUM_10
// to LED_PIN + MAX_DEVICES

// enable components logging
#define LOG_LED 0
#define LOG_I2C 1
#define LOG_EVENTS 1

/* Buttons */
#define PAIR_BUTTON_PIN GPIO_NUM_3
#define SELECT_PREVIOUS_BUTTON_PIN GPIO_NUM_2
#define SELECT_NEXT_BUTTON_PIN GPIO_NUM_4
#define SCREEN_BUTTON_PIN GPIO_NUM_1
#define BUTTON_DEBOUNCE_TIME 50 // ms

/* Display */
#define DISPLAY_WAKEUP_PIN GPIO_NUM_18

/* Storage*/
#define PREFERENCES_PARTITION "MCBC"

/* I2C configuration */
#define I2C_MASTER_SCL_IO           9
#define I2C_MASTER_SDA_IO           8
#define I2C_PORT_NUM                I2C_NUM_1

#define I2C_WIFI_SLAVE_ADDR                0x28

#define I2C_DATA_LEN 32

#endif