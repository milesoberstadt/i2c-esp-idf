# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# Rules to abide
- I will ask you to do one task out of a list of tasks. Please complete my instructions and stop instead of continuing to work ahead on the list of tasks. I need to manually review your work and continuing beyond the scope I ask of you makes this impossible.

# Background

This project aims to implement a multi node WiFi Access Point scanner using ESP32s and SPI for inter device communication. The idea is that SUB nodes are assigned a WiFi channel, they put the wireless chip in promiscious mode on that channel so they can send probe requests and listen to responses without changing channels. The DOM (master) node hops from device to device to collect these records and store them on a microSD card.

# Additional Resource Files

There are other resource files I'll be adding at the root of this project to help compartmentalize context.

* [README.md]() - info on how the hardware is physically setup, how to build
* [TODO.md]() - find our next task to complete
* [RESEARCH.md]() - links to online resources with known good code examples

## Code Style Guidelines
- Include system headers first, then ESP-IDF headers, then local headers
- Use ESP_LOG macros for logging (ESP_LOGI, ESP_LOGW, ESP_LOGE)
- Define constants in constants.h with clear descriptive names
- Use snake_case for function/variable names
- Error handling: Check ESP-IDF function returns with ESP_ERROR_CHECK or explicit checks
- Initialize all variables, use consistent error return patterns
- Use FreeRTOS API for task management and synchronization
- Document functions with clear comments describing purpose and parameters
- Use 2-space indentation, no tabs