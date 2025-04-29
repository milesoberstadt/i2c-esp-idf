#ifndef __SPI_MESSAGES_H__
#define __SPI_MESSAGES_H__

#include <stdbool.h>
#include <stdint.h>
#include "types.h"

/**
 * @brief Get the current WiFi channel
 * @return The current WiFi channel
 */
uint8_t spi_get_wifi_channel(void);

#endif // __SPI_MESSAGES_H__