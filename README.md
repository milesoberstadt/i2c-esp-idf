# wifydra 2.0

This is another wifiydra implemenation using ESP IDF from espressif in the hopes that we can make some improvements.

## Hardware 
Currently the DOM node (an ESP32 WROOM32) can connect to a SUB node (seeed studio esp32s3) via SPI. The connection uses the following pins:

DOM (Master) | SUB (Slave) | Function
-------------|-------------|--------
GPIO 23      | GPIO 9      | MOSI
GPIO 19      | GPIO 8      | MISO
GPIO 18      | GPIO 7      | SCLK
GPIO 5       | GPIO 1      | CS (Chip Select) - SUB node 1
GPIO 4       | GPIO 1      | CS (Chip Select) - SUB node 2
GPIO 21      | GPIO 1      | CS (Chip Select) - SUB node 3
GPIO 22      | GPIO 1      | CS (Chip Select) - SUB node 4
GPIO 32      | GPIO 1      | CS (Chip Select) - SUB node 5
GPIO 33      | GPIO 1      | CS (Chip Select) - SUB node 6
GPIO 25      | GPIO 1      | CS (Chip Select) - SUB node 7
GPIO 26      | GPIO 1      | CS (Chip Select) - SUB node 8
GPIO 27      | GPIO 1      | CS (Chip Select) - SUB node 9
GPIO 2       | GPIO 1      | CS (Chip Select) - SUB node 10
GPIO 16      | GPIO 1      | CS (Chip Select) - SUB node 11

The SPI bus (MOSI, MISO, SCLK) is shared between SUB nodes, with individual CS lines for device selection.

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
