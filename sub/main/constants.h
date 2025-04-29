#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

/* SPI configuration */
#define SPI_HOST              SPI2_HOST
#define SPI_DMA_CHAN          SPI_DMA_CH_AUTO
#define SPI_MOSI_PIN          9  /* MOSI signal (input to slave) */
#define SPI_MISO_PIN          8  /* MISO signal (output from slave) */
#define SPI_SCLK_PIN          7  /* Clock signal */
#define SPI_CS_PIN            44  /* Chip select (slave enable) */

/* Communication parameters */
#define SPI_MAX_TRANSFER_SIZE 32
#define SPI_QUEUE_SIZE        3

/* Wi-Fi channels for sniffer */
#define DEFAULT_WIFI_CHANNEL  1

/* Logging tags */
#define SPI_SLAVE_TAG         "SPI_SLAVE"
#define SPI_MESSAGES_TAG      "SPI_MESSAGES"

#endif // __CONSTANTS_H__