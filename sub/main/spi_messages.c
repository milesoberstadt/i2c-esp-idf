#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "spi_slave.h"
#include "spi_messages.h"
#include "types.h"

#define TAG "SPI_MESSAGES"

// Implementation is mostly in spi_slave.c
// This file is kept for symmetry with the DOM node

uint8_t spi_get_wifi_channel(void) {
    return spi_slave_get_wifi_channel();
}