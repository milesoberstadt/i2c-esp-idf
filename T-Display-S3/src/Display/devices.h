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
        static Devices& instance()
        {
            static Devices instance;
            return instance;
        }

        Devices(const Devices&) = delete;
        Devices& operator=(const Devices&) = delete;

        void set_device_type(uint8_t idx, device_type_t type);
        void set_device_state(uint8_t idx, device_state_t state);
        void set_device_value(uint8_t idx, uint8_t* value, uint8_t value_size);

        device_t get_device(uint8_t idx);
        std::vector<device_t> get_devices();

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
