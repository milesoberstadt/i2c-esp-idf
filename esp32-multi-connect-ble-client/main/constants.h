#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

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
#define SCREEN_BUTTON_PIN GPIO_NUM_15
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

#define I2C_SCREEN_SLAVE_ADDR              0x28
#define I2C_WIFI_SLAVE_ADDR                0x29

#define I2C_BUFFER_SIZE 20

#endif