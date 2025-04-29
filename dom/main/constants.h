#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

/* Logging tags */
#define SPI_TAG                "SPI_MASTER"

/* SPI configuration */
#define SPI_HOST               SPI2_HOST
#define SPI_DMA_CHAN          SPI_DMA_CH_AUTO
#define SPI_MOSI_PIN          23
#define SPI_MISO_PIN          19
#define SPI_SCLK_PIN          18
#define SPI_CS_PIN            5   /* Chip select for the sub node */

/* Communication parameters */
#define SPI_MAX_TRANSFER_SIZE 32
#define SPI_QUEUE_SIZE        1
#define SPI_CLOCK_SPEED_HZ    1000000  /* 1 MHz - slower for stability */

#endif // __CONSTANTS_H__