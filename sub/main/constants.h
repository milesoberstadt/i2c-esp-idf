#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

/* I2C configuration */
#define I2C_SLAVE_SCL_IO           6  /* Changed from 9 to 22 */
#define I2C_SLAVE_SDA_IO           5  /* Changed from 8 to 21 */
#define I2C_PORT_NUM               I2C_NUM_0  /* Now using I2C_NUM_0 */
#define I2C_SLAVE_ADDR_MIN         0x1F  /* Minimum I2C address (31 decimal) for initial randomization */
#define I2C_SLAVE_ADDR_MAX         0x77  /* Maximum I2C address (119 decimal) for initial randomization */

/* The DOM node will reassign addresses in this range after identification */
#define I2C_ASSIGNED_ADDR_MIN      0x0A  /* Minimum assigned I2C address (10 decimal) */
#define I2C_ASSIGNED_ADDR_MAX      0x1E  /* Maximum assigned I2C address (30 decimal) */

#define I2C_DATA_LEN               32

#endif // __CONSTANTS_H__