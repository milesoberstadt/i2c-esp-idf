# wifydra 2.0

This is another wifiydra implemenation using ESP IDF from espressif in the hopes that we can make some improvements.

## Hardware
Currently the DOM node (a Raspberry Pi Pico) can connect to SUB nodes (seeed studio esp32s3) via SPI. The connection uses the following pins:

DOM (Master) | SUB (Slave) | Function
-------------|-------------|--------
GPIO 19      | GPIO 9      | MOSI
GPIO 16      | GPIO 8      | MISO
GPIO 18      | GPIO 7      | SCLK
GPIO 17      | GPIO 1      | CS (Chip Select) - SUB node 1
GPIO 20      | GPIO 1      | CS (Chip Select) - SUB node 2
GPIO 21      | GPIO 1      | CS (Chip Select) - SUB node 3
GPIO 22      | GPIO 1      | CS (Chip Select) - SUB node 4
GPIO 26      | GPIO 1      | CS (Chip Select) - SUB node 5
GPIO 27      | GPIO 1      | CS (Chip Select) - SUB node 6
GPIO 28      | GPIO 1      | CS (Chip Select) - SUB node 7
GPIO 0       | GPIO 1      | CS (Chip Select) - SUB node 8
GPIO 1       | GPIO 1      | CS (Chip Select) - SUB node 9
GPIO 2       | GPIO 1      | CS (Chip Select) - SUB node 10
GPIO 3       | GPIO 1      | CS (Chip Select) - SUB node 11

GP17, GP20, GP21, GP22, GP26, GP27, GP28, GP0, GP1, GP2, GP3
The SPI bus (MOSI, MISO, SCLK) is shared between SUB nodes, with individual CS lines for device selection.

(raspberry pi pico datasheet)[https://datasheets.raspberrypi.com/pico/pico-datasheet.pdf]

(seeed studio esp32s3 spec sheet)[https://www.espboards.dev/esp32/xiao-esp32s3/]

## Build Commands
- Activate esp-idf shell: `source /home/miles/esp/esp-idf/export.sh`
- Build project: `idf.py build` (run from dom/ or sub/ directory)

The following commands should not be run by an agent:

- Flash to device: `idf.py -p PORT flash` (replace PORT with device port)
- Monitor output: `idf.py -p PORT monitor`
- Erase NVS: `./erase_nvs.sh` (in sub/ directory)
- Monitor serial: `./monitor.sh` (in either dom/ or sub/ directory)
