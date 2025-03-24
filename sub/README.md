# WiFi Scanner SUB Node

The SUB node is a WiFi scanner that operates in promiscuous mode to capture WiFi access points and network traffic. It communicates with a central DOM node via I2C for configuration and data transfer.

## Features

- Automatic discovery protocol for DOM-SUB communication
- WiFi promiscuous mode for capturing access points and EAPOL packets
- Memory-efficient storage of discovered access points
- Channel-specific WiFi scanning (assigned by DOM)
- Synchronized timestamp with DOM node

## Hardware Requirements

- ESP32 development board
- I2C connection to DOM node (SDA: GPIO 8, SCL: GPIO 9)

## Building and Flashing

```bash
# Configure the project
idf.py menuconfig

# Build, flash, and monitor
idf.py -p PORT flash monitor
```

## Architecture

The SUB node implements:

1. **I2C Slave**: Communicates with DOM node over I2C
2. **WiFi Scanner**: Operates in promiscuous mode to capture WiFi packets
3. **AP List**: Stores and manages discovered access points
4. **Main Task**: Coordinates operations and reports status

## Operation

1. On startup, the SUB node generates a random 2-character ID
2. It joins a random I2C address in the discovery range (20-127)
3. When the DOM node sends a "hello" message, the SUB responds with its ID
4. DOM verifies the SUB and assigns it a permanent I2C address (1-19) and WiFi channel
5. The SUB then scans the assigned WiFi channel in promiscuous mode
6. When an access point is detected, it's stored in memory
7. The DOM periodically retrieves the discovered access points
8. The SUB marks APs as synchronized once confirmed by the DOM

## Future Enhancements

- Full EAPOL packet capture and storage
- Enhanced promiscuous mode features
- Support for fast channel hopping
- Battery monitoring and power saving features