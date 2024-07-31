#if !defined(__SLAVE_HPP__)
#define __SLAVE_HPP__

#include <Wire.h>
#include <Arduino.h>
#include "types.h"
#include "devices.h"
#include "utils.h"
#include "sleep.h"

// Pin configuration for I2C on T-Display S3
#define I2C_SLAVE_SDA 8
#define I2C_SLAVE_SCL 9
#define I2C_SLAVE_ADDR 0x28

class I2CSlave
{
    public:
        static void on_receive_static(int byteCount);
        static void on_request_static();
        static I2CSlave& instance()
        {
            static I2CSlave instance;
            return instance;
        }
        I2CSlave(const I2CSlave &) = delete;
        I2CSlave &operator=(const I2CSlave &) = delete;

    private:
        void on_receive(int byteCount);
        void on_request();
        void process_message(uint8_t *data, size_t length);
        I2CSlave();
        ~I2CSlave();
};

#endif // __SLAVE_HPP__
