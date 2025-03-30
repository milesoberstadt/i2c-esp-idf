# DOM Node (I2C Master)

This ESP-IDF project implements an I2C master node that communicates with a SUB node via I2C.

## Features

- I2C master implementation for communicating with SUB node
- Structured message exchange protocol
- Periodic sending of dummy data and data requests

## Building and Flashing

To build and flash the project:

```bash
# Navigate to the project directory
cd dom

# Build the project
idf.py build

# Flash to your ESP32 device (replace PORT with your device's port)
idf.py -p PORT flash

# Monitor the output
idf.py -p PORT monitor
```

## I2C Communication

The DOM node acts as an I2C master and communicates with the SUB node (slave) using a simple message protocol:

- Fixed length 32-byte messages
- 3-byte header (message type, device index, data length)
- Variable-length payload
- Unused bytes padded with 0xFF

## Message Types

- `msg_init_start/end`: Initialization sequence
- `msg_data`: Send data to the SUB node
- `msg_req_data`: Request data from the SUB node
- `msg_res_data`: Response data from the SUB node

## Hardware Setup

Connect the DOM and SUB nodes as follows:

- SCL: GPIO 9 on both devices
- SDA: GPIO 8 on both devices
- GND: Common ground between devices