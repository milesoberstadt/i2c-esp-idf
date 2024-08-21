# nRF52840 Sensor Server

Look at the main [README.md](../README.md) for an overview of the project to understand the context of this project.

<image src="../images/nodes.png">

Use Arduino IDE to compile and upload the code to the XIAO BLE Sense.

### Hardware used :

- [Seeed Studio XIAO nRF52840 (Sense)](https://wiki.seeedstudio.com/XIAO_BLE/)
- [Custom PCB](measure_board.pdf) (optional)

> A-Node only :

- [RS485 to TTL](https://techfun.sk/produkt/obojsmerny-prevodnik-ttl-usart-na-rs485/)
- [RS-FXJT-N01 Wind direction transmitter](https://instrucenter.com/wp-content/uploads/2022/03/RS-FXJT-N01.pdf)
- [RS-FSJT-N01 Wind speed transmitter](https://instrucenter.com/wp-content/uploads/2022/03/RS-FSJT-No1.pdf)

### Libraries used:

- [ArduinoBLE](https://www.arduino.cc/en/Reference/ArduinoBLE)
- [Seeed Arduino LSM6DS3](https://github.com/Seeed-Studio/Seeed_Arduino_LSM6DS3/)

## Configuration

### Node type

This code is used for the M-Node, A-Node and S-Node.
Define `NODE_TYPE` to `M_NODE`, `A_NODE` or `S_NODE` to compile the code for the M-Node, A-Node or S-Node.

### Button

If you want to use the button, define `USE_BUTTON` to `1`.

See [Button](#how to use) for more information on how to use the button.

### Pinout

`BUTTON_PIN` and `SLEEP_PIN` can be changed in the `nrf-sensor-server.ino` file.

Default values if using the custom PCB:

- `BUTTON_PIN` = 2 (correspond to BTN S2)
- `SLEEP_PIN` = D0

### Battery level

Every `BATTERY_SEND_INTERVAL` (ms), the battery level is sent to the Main-Node.
You can change the interval, as well as the battery level calculation in the `battery.ino` file.

### Other options.

BLE service UUIDs and characteristics UUIDs can be changed in the `nrf-sensor-server.ino` file.

Bluetooth discovery duration, as well as the sleep duration, can be changed in the `nrf-sensor-server.ino` file.

## How to use

### General usage

**LED**

The Serial LED is used to indicate the state of the node.

- Slow 🔴 blink : disconnected
- Fast 🟢 blink : pairing (device is discoverable)
- Slow 🟢 blink : connected

**Button**

`BT_User` (marked as S2 on the PCB) can be used to start the pairing process when device is disconnected.

_Note :_ If `USE_BUTTON` = 0, the the device will always be in pairing mode while not connected.

### M-Node

The M-Node will send the gyro and accelerometer data to the Main-Node when connected at a `SEND_INTERVAL` rate (Hz).

It can be put to sleep by putting a LOW signal on the `SLEEP_PIN` pin. It can be woken up by putting a HIGH signal on the `SLEEP_PIN` pin.

### A-Node

The A-Node will send the wind speed and direction to the Main-Node when connected at a `SEND_INTERVAL` rate (Hz).

_Note :_ The send interval might not be respected because of the delay required to read the wind speed and direction using RS485.

### S-Node

The S-Node will listen to the Sleep BLE Characteristic (uuid: `2c41ce1f-acd3-4088-8394-b21a88e88142`) of the Sleep Service (uuid `63e4eb54-b0bc-4374-8d2a-5f08f951230a`).

When the value is TRUE, it will put the M-Node to sleep by putting a LOW signal on the `SLEEP_PIN` pin. When the value is FALSE, it will wake up the M-Node by putting a HIGH signal on the `SLEEP_PIN` pin.

## Understanding the code

The code is split into 4 files :

- `nrf-sensor-server.ino` : main file, contains the setup and loop functions.
- `battery.ino` : contains the battery level calculation and sending.
- `a-node.ino` : contains the A-Node specific code. (for RS485 communication)
- `leds.ino` : contains the LED functions for controlling serial LED.

#### `nrf-sensor-server.ino`

The setup function will initialize the BLE service and characteristics, the battery level, the button and the sleep pin.

The loop function will handle the BLE events, the button press, the sleep signal and the battery level sending. It will call the node specific functions (M-Node, A-Node or S-Node) to get and send the data.

#### `battery.ino`

The battery level is calculated using the voltage divider method. The battery level is sent to the Main-Node every `BATTERY_SEND_INTERVAL` ms.

#### `a-node.ino`

The A-Node specific code is in this file. The `RS-FXJT-N01` Wind direction transmitter and the `RS-FSJT-N01` Wind speed transmitter are read using RS485 communication. They wait for a specific frame to be sent to start sending the data.

## Authors

- [**jansumsky**](https://github.com/jansumsky): project manager, hardware decisions
- **Tomas Baca**: hardware conception
- [**franckg28**](https://github.com/FranckG28): software developer & hardware testing
- [**max1lock**](https://github.com/max1lock): S-Node, researches and trials on i2c and wifi.
- [**leHofficiel**](https://github.com/leHofficiel): researches
- [**alxandre-r**](https://github.com/alxandre-r): researches
