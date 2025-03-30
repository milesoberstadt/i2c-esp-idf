#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

/* I2C configuration */
#define I2C_MASTER_SCL_IO           22  /* Changed from 9 to 22 */
#define I2C_MASTER_SDA_IO           21  /* Changed from 8 to 21 */
#define I2C_PORT_NUM                I2C_NUM_0  /* Now using I2C_NUM_0 */

#define I2C_SUB_SLAVE_ADDR          0x28

#define I2C_DATA_LEN                32

#endif // __CONSTANTS_H__