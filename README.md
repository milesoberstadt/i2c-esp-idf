# wifydra 2.0

This is another wifiydra implemenation using ESP IDF from espressif in the hopes that we can make some improvements.

## Hardware 
Currently the DOM node (an ESP32 WROOM32) can connect to a SUB node (seeed studio esp32s3) via SPI. The connection uses the following pins:

DOM (Master) | SUB (Slave) | Function
-------------|-------------|--------
GPIO 23      | GPIO 9      | MOSI
GPIO 19      | GPIO 8      | MISO
GPIO 18      | GPIO 7      | SCLK
GPIO 5       | GPIO 44     | CS (Chip Select)

Currently the CS line is tied directly from the DOM node to the SUB node

(esp32 wroom spec sheet)[https://www.espboards.dev/esp32/nodemcu-32s/]

(seeed studio esp32s3 spec sheet)[https://www.espboards.dev/esp32/xiao-esp32s3/]

## Build Commands
- Activate esp-idf shell: `source /home/miles/esp/esp-idf/export.sh`
- Build project: `idf.py build` (run from dom/ or sub/ directory)

The following commands should not be run by an agent:

- Flash to device: `idf.py -p PORT flash` (replace PORT with device port)
- Monitor output: `idf.py -p PORT monitor`
- Erase NVS: `./erase_nvs.sh` (in sub/ directory)
- Monitor serial: `./monitor.sh` (in either dom/ or sub/ directory)
