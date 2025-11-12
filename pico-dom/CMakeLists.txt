cmake_minimum_required(VERSION 3.13)

# Initialize the Pico SDK
set(PICO_SDK_PATH "${PICO_SDK_PATH}")
include(pico_sdk_import.cmake)

project(pico_dom C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

add_executable(pico_dom
    main.c
)

# Pull in our pico_stdlib which aggregates commonly used features
target_link_libraries(pico_dom
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_timer
    pico_multicore
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(pico_dom 1)
pico_enable_stdio_uart(pico_dom 0)

# Create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(pico_dom)