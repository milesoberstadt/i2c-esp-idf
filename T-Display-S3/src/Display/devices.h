#if !defined(__DEVICES_H__)
#define __DEVICES_H__

#include <vector>
#include <stdint.h>
#include <memory>
#include <algorithm>

#include "types.h"
#include "devices_observable.h"
#include "devices_observer.h"

class Devices: public DevicesObservable {
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

        void attach(DevicesObserver *observer);
        void detach(DevicesObserver *observer);
        void notify();

    private:
        Devices();
        ~Devices();
        std::vector<std::shared_ptr<DevicesObserver>> observers;
        std::vector<device_t> devices;
    
};

#endif // __DEVICES_H__
