#ifndef __SPI_MESSAGES_H__
#define __SPI_MESSAGES_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "types.h"

/**
 * @brief Send a message to a specific device
 * @param msg_type The message type to send
 * @param device_index Target device index
 * @return true if successful, false otherwise
 */
bool spi_msg_send(message_t msg_type, uint8_t device_index);

/**
 * @brief Send a message with data to a specific device
 * @param msg_type The message type to send
 * @param device_index Target device index
 * @param data Pointer to data buffer
 * @param data_len Length of data
 * @return true if successful, false otherwise
 */
bool spi_msg_send_data(message_t msg_type, uint8_t device_index, const uint8_t *data, size_t data_len);

/**
 * @brief Request data from a specific device
 * @param device_index Target device index
 * @param rx_data Pointer to receive buffer
 * @param rx_data_len Pointer to receive length (in/out parameter)
 * @return true if successful, false otherwise
 */
bool spi_msg_request_data(uint8_t device_index, uint8_t *rx_data, size_t *rx_data_len);

/**
 * @brief Set WiFi channel for a specific device
 * @param device_index Target device index
 * @param channel WiFi channel (1-14)
 * @return true if successful, false otherwise
 */
bool spi_msg_set_wifi_channel(uint8_t device_index, uint8_t channel);

#endif // __SPI_MESSAGES_H__