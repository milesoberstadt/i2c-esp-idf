# Main Node (ESP-IDF)

This project is the Main-Node of the [Tree Monitoring System](../README.md).

> Illustration goes here

The Main-Node is placed on the main board, on the ground. It is used to collect the data from the M-Node and A-Node, and to send sleep/wakeup signal to S-Node. This is the central node of the network.

This node is meant to be used together with the [Wifi node](../wifi-node/README.md).

## Features

- ⚡️ Connect & collect data from up to 8 devices (from different types)
- 🛜 Transmit data to the Wifi-Node
- 🖥️ Monitor the system using 6 buttons and 8 LEDs (one per connected device)
- 📡 Automatic detection of available bluetooth devices (using their advertised service UUID)
- 💾 Automatic reconnection to last connected devices
- ⚙️ Customizable devices types, BLE services and characteristics
- ☎️ Flexible Button management, allow registering press and long-press events on buttons.
- 💡 Flexible LED management, allowing to set the LED state and blink rate.
- 🗂️ Devices state "cache" to keep track of the last known state of each device

## Requirements

### Hardware used

- [ESP32-S3](https://www.espressif.com/en/products/socs/esp32-s3)
- 6 buttons
- 8 LEDs

### Software used

- [ESP-IDF 5.3](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/index.html)

## How to Use

On startup, the Main-Node will automatically try to connect to the last known devices.

<image src="./images/leds.png" width="500px" />

One LED represents the status of one device.

- Off : device not connected
- Slow blink : pairing
- Fast blink : trying to connect
- On : connected

Use the buttons to interact with the system.

<image src="./images/buttons.png" width="600px" />

- **Toggle screen** : switch the screen ON/OFF
- **Previous** and **Next** : select the device to interact with. The LED of the selected device will blink once.
- **Select** :
  - Simple press : try to reconnect to the last connected device at this position
  - Long press : start pairing for device at this position (a successful pairing will overwrite the last connected device at this position)

## Configuration

### General configuration

Defined in [`constants.h`](./main-node/main/constants.h)

```c
/* Maximum connected devices count */
#define MAX_DEVICES 8
// esp32-s3 supports up to 9 ACL connections

/* BLE configuration */
#define PAIRING_DURATION 30 // seconds

// led pins start from here
#define LED_PIN GPIO_NUM_10
// to LED_PIN + MAX_DEVICES-1

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

/* Storage*/
#define PREFERENCES_PARTITION "MCBC"

/* I2C configuration */
#define I2C_MASTER_SCL_IO           9
#define I2C_MASTER_SDA_IO           8
#define I2C_PORT_NUM                I2C_NUM_1

#define I2C_WIFI_SLAVE_ADDR                0x28

#define I2C_DATA_LEN 32

```

For the LEDs, the pins are defined in the `LED_PIN` constant. The first LED will be connected to the pin defined in `LED_PIN`, the second to `LED_PIN + 1`, and so on.

<image src="./images/leds-pinout.png" width="120px" />

### Nodes configuration

Defined in [`device_config.h`](./main-node/main/device_config.h)

Example :

```c
// Give a unique ID to each device type
typedef enum device_type_t {
    UNKNOWN_DEVICE = -1,
    DEVICE_M_NODE = 0,
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

static esp_bt_uuid_t M_NODE_CHAR_UUIDS[] = { UUID_GYRO_CHAR, UUID_ACCEL_CHAR };

/* --- Device configurations --- */

static const device_type_config_t M_NODE_CONFIG = {
        .service_uuid = UUID_M_NODE,
        .char_uuids = M_NODE_CHAR_UUIDS,
        .char_count = 2,
    };

static const device_type_config_t DEVICE_CONFIGS[] = {
    [DEVICE_M_NODE] = M_NODE_CONFIG,
};
```

**Important note** :
uuids bits are reversed in the `uuid128` array. For example, the UUID `19b10000-e8f2-537e-4f6c-d104768a1214` will be written as `{ 0x14, 0x12, 0x8a, 0x76, 0x04, 0xd1, 0x6c, 0x4f, 0x7e, 0x53, 0xf2, 0xe8, 0x00, 0x00, 0xb1, 0x19 }`.

## Development

Before project configuration and build, be sure to set the correct chip target using:

```bash
idf.py set-target <chip_name>
```

The code can be modified to connect to more devices (up to 4 devices by default). If you need to connect to more devices (more than 4 devices), you need to change `BT/BLE MAX ACL CONNECTIONS` in menuconfig.

#### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type `Ctrl-]`.)

See the [Getting Started Guide](https://idf.espressif.com/) for full steps to configure and use ESP-IDF to build projects.

## Understanding the Code

...

## Authors

- [**jansumsky**](https://github.com/jansumsky): project manager, hardware decisions
- **Tomas Baca**: hardware conception
- [**franckg28**](https://github.com/FranckG28): software developer & hardware testing
- [**max1lock**](https://github.com/max1lock): S-Node, researches and trials on i2c and wifi.
- [**leHofficiel**](https://github.com/leHofficiel): researches
- [**alxandre-r**](https://github.com/alxandre-r): researches
