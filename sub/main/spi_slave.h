#ifndef __SPI_SLAVE_H__
#define __SPI_SLAVE_H__

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize the SPI slave interface
 * @return true if initialization was successful, false otherwise
 */
bool spi_slave_init(void);

/**
 * @brief Deinitialize the SPI slave interface
 */
void spi_slave_deinit(void);

/**
 * @brief Start SPI slave tasks for processing transactions
 * @return true if tasks were started successfully, false otherwise
 */
bool spi_slave_start_tasks(void);

/**
 * @brief Get the current WiFi channel
 * @return The current WiFi channel
 */
uint8_t spi_slave_get_wifi_channel(void);

#endif // __SPI_SLAVE_H__