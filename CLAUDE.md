# Tree Sway Monitoring System - Development Guide

## Build Commands
- ESP32 nodes: `idf.py -p PORT flash monitor` (Build, flash and monitor)
- ESP32 config: `idf.py menuconfig` (Configure WiFi settings etc.)
- ESP32 target: `idf.py set-target <chip_name>` (Set target chip)
- NRF nodes: Use Arduino IDE to compile and upload code to XIAO BLE Sense

## Code Style Guidelines
- Naming: snake_case for functions/variables, UPPER_CASE for constants, PascalCase for types
- Imports: Group by standard libs, ESP-IDF libs, FreeRTOS, then project includes
- Error handling: Use ESP_ERROR_CHECK for ESP-IDF functions, check return values
- Memory: Free all allocated memory (malloc/free pairs)
- I2C Protocol: Fixed 3-byte header (msg_type, dev_idx, data_len) with variable payload
- BLE: Use 128-bit UUIDs with bits reversed in uuid128 arrays

## Project Structure
- main-node: ESP32-S3 BLE central that collects data from sensors
- wifi-node: ESP32-S3 that handles display, SD card, and WiFi
- nrf-sensor-server: XIAO BLE Sense code for M-Node, A-Node, and S-Node
- Communication: BLE between sensors and main-node, I2C between main-node and wifi-node