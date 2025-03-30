# SUB Node (I2C Slave)

This ESP-IDF project implements an I2C slave node that communicates with a DOM node via I2C.

## Features

- I2C slave implementation for receiving commands from DOM node
- Structured message exchange protocol
- Ability to send response data back to the master

## Building and Flashing

To build and flash the project:

```bash
# Navigate to the project directory
cd sub

# Build the project
idf.py build

# Flash to your ESP32 device (replace PORT with your device's port)
idf.py -p PORT flash

# Monitor the output
idf.py -p PORT monitor
```

## I2C Communication

The SUB node acts as an I2C slave and communicates with the DOM node (master) using a simple message protocol:

- Fixed length 32-byte messages
- 3-byte header (message type, device index, data length)
- Variable-length payload
- Unused bytes padded with 0xFF

## Message Types

- `msg_init_start/end`: Initialization sequence
- `msg_data`: Receive data from the DOM node
- `msg_req_data`: Respond to data requests from the DOM node
- `msg_res_data`: Send response data to the DOM node

## Hardware Setup

Connect the DOM and SUB nodes as follows:

- SCL: GPIO 9 on both devices
- SDA: GPIO 8 on both devices
- GND: Common ground between devices