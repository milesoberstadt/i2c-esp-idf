# WiFi Scanner with GPS Triangulation

This project implements a distributed WiFi scanning system capable of detecting access points and capturing WiFi traffic across multiple channels simultaneously. The system is designed for wireless network mapping, triangulation, and passive monitoring.

## System Architecture

The system consists of two types of nodes:

### DOM Node (Dominant)
- Central coordinator that manages all SUB nodes
- Collects data from SUBs via I2C
- Assigns WiFi channels to SUBs
- Synchronizes timestamps across the system
- Stores and processes collected data

### SUB Nodes (Subordinate)
- WiFi scanners that operate in promiscuous mode
- Each SUB is assigned a specific WiFi channel
- Detect and capture WiFi access points and traffic
- Store data locally and sync with the DOM node
- Automatically discovered and configured by DOM

## Features

- **Multi-Channel Scanning**: Simultaneously scan all WiFi channels (1-11)
- **Access Point Detection**: Discover and record WiFi networks
- **EAPOL Packet Capture**: Capture authentication handshakes (planned)
- **Automatic Discovery**: SUBs are automatically detected and configured
- **I2C Communication**: Efficient data transfer between nodes
- **Timestamp Synchronization**: Coordinated timing across all nodes

## Hardware Requirements

- ESP32 development boards (1 DOM node, multiple SUB nodes)
- I2C connections between DOM and SUBs (SDA: GPIO 8, SCL: GPIO 9)
- (Planned) GPS module for DOM node
- (Planned) SD card for data storage

## Building and Flashing

### DOM Node
```bash
cd dom
idf.py build
idf.py -p PORT flash monitor
```

### SUB Nodes
```bash
cd sub
idf.py build
idf.py -p PORT flash monitor
```

## Project Structure

- `/dom` - ESP-IDF project for the DOM node
- `/sub` - ESP-IDF project for the SUB nodes

## Future Enhancements

- GPS module integration for location tracking
- SD card support for extended data logging
- Full EAPOL packet capture and analysis
- Web interface for system monitoring and control
- Triangulation based on signal strength from multiple SUBs
