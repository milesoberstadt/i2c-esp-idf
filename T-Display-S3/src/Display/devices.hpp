#if !defined(__DEVICES_HPP__)
#define __DEVICES_HPP__

#include <vector>

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

        void addDevice(device_t device);
        void removeDevice(device_t device);
        void updateDevice(device_t device);
        void updateDeviceState(device_t device, device_state_t state);
        void updateDeviceValue(device_t device, uint8_t* value, uint8_t value_size);
        device_t getDevice(device_type_t type);
        std::vector<device_t> getDevices();

    private:
        Devices();
        ~Devices();
        std::vector<device_t> devices;
    
};

#endif // __DEVICES_HPP__
