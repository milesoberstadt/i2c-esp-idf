#if !defined(__SLAVE_HPP__)
#define __SLAVE_HPP__

#include <Wire.h>
#include <Arduino.h>
#include "types.h"
#include "devices.h"
#include "utils.h"

// Pin configuration for I2C on T-Display S3
#define I2C_SLAVE_SDA 8
#define I2C_SLAVE_SCL 9
#define I2C_SLAVE_ADDR 0x28

class I2CSlave
{
public:
    void begin();
    static void onReceiveStatic(int byteCount);
    static void onRequestStatic();
    static I2CSlave &getInstance()
    {
        static I2CSlave instance;
        return instance;
    }
    I2CSlave(const I2CSlave &) = delete;
    I2CSlave &operator=(const I2CSlave &) = delete;

private:
    void onReceive(int byteCount);
    void onRequest();
    void processMessage(uint8_t *data, size_t length);
    I2CSlave();
    ~I2CSlave();
};

inline I2CSlave &i2cSlaveInstance()
{
    return I2CSlave::getInstance();
}
#endif // __SLAVE_HPP__
