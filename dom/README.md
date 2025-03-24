# WiFi Scanner DOM Node

The DOM (Dominant) node is the central coordinator for the WiFi scanning system. It communicates with multiple SUB nodes via I2C, coordinates WiFi channel scanning, and collects access point data.

## Features

- Automatic discovery and configuration of multiple SUB nodes
- I2C communication with SUBs for control and data collection
- Synchronized timestamps across all SUBs
- Periodic collection and storage of discovered WiFi access points

## Hardware Requirements

- ESP32 development board
- I2C connections to SUB nodes (SDA: GPIO 8, SCL: GPIO 9)

## Building and Flashing

```bash
# Configure the project
idf.py menuconfig

# Build, flash, and monitor
idf.py -p PORT flash monitor
```

## Architecture

The DOM node implements:

1. **I2C Master**: Communicates with SUB nodes over I2C
2. **SUB Manager**: Discovers, configures, and manages SUB nodes
3. **AP Database**: Stores and manages discovered access points
4. **Synchronization Task**: Periodically collects data from SUBs
5. **Console Task**: Displays system status via serial console

## Operation

1. On startup, the DOM node scans the I2C bus for SUB nodes
2. SUBs are assigned unique I2C addresses and WiFi channels
3. DOM sets timestamps on all SUBs to maintain synchronization
4. SUBs are commanded to start scanning on their assigned channels
5. DOM periodically polls SUBs to collect detected access points
6. DOM displays system status and collected data on the console

## Future Enhancements

- GPS module integration for accurate timestamps and positioning
- SD card support for data logging
- Web interface for remote management
- EAPOL packet capture for network analysis