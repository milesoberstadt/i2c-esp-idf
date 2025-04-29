# wifydra 2.0

This is another wifiydra implemenation using ESP IDF from espressif in the hopes that we can make some improvements.

Warning: A lot of this code was writting with help from Claude Code, I did my best to review everything it wrote, but there could be some garbage in here.

## Hardware 
Currently the DOM node (an ESP32 WROOM32) can connect to a SUB node (seeed studio esp32s3) via SPI. The connection uses the following pins:

DOM (Master) | SUB (Slave) | Function
-------------|-------------|--------
GPIO 23      | GPIO 13     | MOSI
GPIO 19      | GPIO 12     | MISO
GPIO 18      | GPIO 14     | SCLK
GPIO 5       | GPIO 15     | CS (Chip Select)

![wiring_diagram](./images/i2c_wiring_diagram.png)

Note: The current wiring diagram shows I2C connections; an updated SPI diagram will be provided.

## Communication Protocol
- Fixed length 32-byte messages
- Message format:
  - Byte 0: Message type
  - Byte 1: Device index
  - Byte 2: Data length
  - Bytes 3+: Payload data
- Unused bytes padded with 0xFF

## Message Types
- `msg_init_start/end`: Initialization sequence
- `msg_data`: Send data to the SUB node
- `msg_req_data`: Request data from the SUB node
- `msg_res_data`: Response data from the SUB node
- `msg_req_identifier`: Request identifier from the SUB node
- `msg_res_identifier`: Identifier response from the SUB node
- `msg_set_wifi_channel`: Set WiFi channel for sniffer

## Credits

- [**franckg28**](https://github.com/FranckG28/measuring-tree-sway): This was the foundation of my i2c communication on ESP IDF, I could not have gotten this working without that foundation.