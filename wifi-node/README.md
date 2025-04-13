# Wifi node

Look at the main [README.md](../README.md) for an overview of the project to understand the context of this project.

<image src="../images/wifi-node.png">

The Wifi-Node is placed on the main board, on the ground. It is used to send the data collected by the Main-Node to the cloud, or write the data to a SD card.
It is also the one that control the display.

We use a esp32-s3 for his Wifi capabilities, and his low power consumption.

This node is meant to be used together with the [Main node](../main-node/README.md).

## Features

- 🛜 Connect to a Wifi AP
- 💾 Write to SD card
- 🖥️ Display selected device info on the screen
- 📡 Listening to i2c messages

## Requirements

### Hardware used

- [ESP32-S3](https://www.espressif.com/en/products/socs/esp32-s3)
- Another microcontroller that send data through i2c
- [SDD1306 OLED display](https://www.adafruit.com/product/938)
- [SD card module](https://www.adafruit.com/product/254)

### Software used

- [ESP-IDF 5.3](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/index.html)

## How to Use

Before compiling the code, you need to configure the Wifi-Node using `idf.py menuconfig`.

```bash
idf.py menuconfig
```

In the `Example Configuration` menu, you can set the Wifi SSID and password.

On startup, the Wifi-Node will automatically try to connect access point defined in menuconfig.

The Wifi-Node will also listen to i2c messages from the Main-Node to display the data on the screen.

This node can't be used alone, it needs to be used with the Main-Node.

## Configuration

### General configuration

The following configuration can be changed in the [`wifi-node/main/constants.h`](./main/constants.h) file.

```c
#define PIN_NUM_MISO  13
#define PIN_NUM_MOSI  11
#define PIN_NUM_CLK   12
#define PIN_NUM_CS    10

#define I2C_SLAVE_SCL_IO           9
#define I2C_SLAVE_SDA_IO           8
#define I2C_PORT_NUM               I2C_NUM_1
#define I2C_SLAVE_ADDR             0x28

#define I2C_DATA_LEN 32

#define LOG_I2C_MESSAGES 1

#define DEVICES_COUNT 8
```

## Development

Before project configuration and build, be sure to set the correct chip target using:

```bash
idf.py set-target <chip_name>
```

#### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type `Ctrl-]`.)

See the [Getting Started Guide](https://idf.espressif.com/) for full steps to configure and use ESP-IDF to build projects.

## Credits

- [**jansumsky**](https://github.com/jansumsky): project manager, hardware decisions
- **Tomas Baca**: hardware conception
- [**franckg28**](https://github.com/FranckG28): software developer & hardware testing
- [**max1lock**](https://github.com/max1lock): S-Node, researches and trials on i2c and wifi.
- [**leHofficiel**](https://github.com/leHofficiel): researches
- [**alxandre-r**](https://github.com/alxandre-r): researches
- [**nopnop2002**](https://github.com/nopnop2002/esp-idf-ssd1306): esp-idf-ssd1306 library
