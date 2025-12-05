# Raspberry Pi Pico DOM Node

This is a port of the ESP32 DOM (master) node to the Raspberry Pi Pico platform.

## Hardware Connections

### SPI Connections
- **MISO**: GP16 (Pin 21)
- **MOSI**: GP19 (Pin 25)
- **CLK**: GP18 (Pin 24)

### Chip Select (CS) Pins
- **CS0**: GP17 (Pin 22) - SUB Node 1
- **CS1**: GP20 (Pin 26) - SUB Node 2

## Pin Mapping Comparison

| Function | ESP32 Pin | Pico GPIO | Pico Pin |
|----------|-----------|-----------|----------|
| SPI MISO | 19        | GP16      | 21       |
| SPI MOSI | 23        | GP19      | 25       |
| SPI CLK  | 18        | GP18      | 24       |
| CS0      | 5         | GP17      | 22       |
| CS1      | 4         | GP20      | 26       |

## Key Differences from ESP32 Version

### Architecture Changes
- **Single Threaded**: Uses a simple main loop with periodic function calls instead of multiple cores/tasks
- **No FreeRTOS**: Uses Pico SDK's simple timing functions instead of FreeRTOS tasks
- **Logging**: Simplified logging using printf instead of ESP-IDF logging system
- **Timer**: Uses Pico SDK's timer functions for periodic execution

### API Differences
- `spi_write_read_blocking()` instead of `spi_device_transmit()`
- `gpio_put()` instead of `gpio_set_level()`
- `sleep_ms()` instead of `vTaskDelay()`
- `to_ms_since_boot()` for timing intervals

## Building

1. Set up the Pico SDK environment
2. Set the `PICO_SDK_PATH` environment variable
3. Build the project:

```bash
mkdir build
cd build
cmake ..
make
```

This will generate `pico_dom.uf2` which can be copied to the Pico when in BOOTSEL mode.

## Functionality

The Pico DOM node maintains the same core functionality as the ESP32 version:

1. **SPI Master Communication**: Polls SUB nodes via SPI to collect WiFi AP counts
2. **Multi-Node Support**: Supports multiple SUB nodes with individual CS pin control
3. **Error Detection**: Detects floating bus conditions and validates responses
4. **Status Logging**: Regular uptime logging and communication status

The polling cycle runs every 10 seconds, querying each connected SUB node for WiFi AP counts.