#ifndef __SPI_MASTER_H__
#define __SPI_MASTER_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "types.h"

/**
 * @brief Initialize the SPI master interface
 * @return true if initialization was successful, false otherwise
 */
bool spi_master_init(void);

/**
 * @brief Deinitialize the SPI master interface
 */
void spi_master_deinit(void);

/**
 * @brief Exchange data with the SPI slave device
 * @param tx_data Pointer to transmit data buffer
 * @param rx_data Pointer to receive data buffer
 * @param len Length of data to transmit/receive
 * @return true if transaction was successful, false otherwise
 */
bool spi_exchange_data(uint8_t *tx_data, uint8_t *rx_data, size_t len);

/**
 * @brief Check if the SPI slave device is responsive
 * @return true if slave device is responsive, false otherwise
 */
bool spi_check_slave(void);

/**
 * @brief Send a message to the SPI slave device without data
 * @param msg_type The message type to send
 * @param device_index Index of the target device
 * @return true if message was sent successfully, false otherwise
 */
bool spi_send_message(message_t msg_type, uint8_t device_index);

/**
 * @brief Send a message with data to the SPI slave device
 * @param msg_type The message type to send
 * @param device_index Index of the target device
 * @param data Pointer to data buffer
 * @param data_len Length of data
 * @return true if message was sent successfully, false otherwise
 */
bool spi_send_message_data(message_t msg_type, uint8_t device_index, const uint8_t *data, size_t data_len);

/**
 * @brief Request data from the SPI slave device
 * @param device_index Index of the target device
 * @param rx_data Pointer to receive buffer
 * @param rx_data_len Pointer to receive length (in/out parameter)
 * @return true if data was received successfully, false otherwise
 */
bool spi_request_data(uint8_t device_index, uint8_t *rx_data, size_t *rx_data_len);

/**
 * @brief Set the WiFi channel for the SPI slave device
 * @param channel WiFi channel (1-14)
 * @return true if channel was set successfully, false otherwise
 */
bool spi_set_wifi_channel(uint8_t channel);

#endif // __SPI_MASTER_H__