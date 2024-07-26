#if !defined(__DEVICES_HPP__)
#define __DEVICES_HPP__

#include <vector>
#include <stdint.h>

#include "types.hpp"

class Devices {
    public:
        static Devices& getInstance()
        {
            static Devices instance;
            return instance;
        }

        Devices(const Devices&) = delete;
        Devices& operator=(const Devices&) = delete;

        void setDeviceState(uint8_t idx, device_state_t state);
        void setDeviceValue(uint8_t idx, uint8_t* value, uint8_t value_size);

        device_t getDevice(uint8_t idx);
        std::vector<device_t> getDevices();

    private:
        Devices();
        ~Devices();

        std::vector<device_t> devices;
    
};

#endif // __DEVICES_HPP__
