#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

/* I2C configuration */
#define I2C_SLAVE_SCL_IO           6  /* Changed from 9 to 22 */
#define I2C_SLAVE_SDA_IO           5  /* Changed from 8 to 21 */
#define I2C_PORT_NUM               I2C_NUM_0  /* Now using I2C_NUM_0 */
#define I2C_SLAVE_ADDR_MIN         0x14  /* Minimum I2C address (20 decimal) */
#define I2C_SLAVE_ADDR_MAX         0x78  /* Maximum I2C address (120 decimal) */

#define I2C_DATA_LEN               32

#endif // __CONSTANTS_H__