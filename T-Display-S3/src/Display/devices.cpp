#include "devices.h"

Devices::Devices()
{
    devices = std::vector<device_t>(DEVICE_COUNT);
}

Devices::~Devices()
{
    devices.clear();
    observers.clear();
}

void Devices::set_device_state(uint8_t idx, device_state_t state)
{
    devices[idx].state = state;
    notify();
}

void Devices::set_device_value(uint8_t idx, uint8_t* value, uint8_t value_size)
{
    devices[idx].value = value;
    devices[idx].value_size = value_size;
    notify();
}

void Devices::set_device_type(uint8_t idx, device_type_t type)
{
    devices[idx].type = type;
    notify();
}

device_t Devices::get_device(uint8_t idx)
{
    return devices[idx];
}

std::vector<device_t> Devices::get_devices()
{
    return devices;
}

void Devices::attach(DevicesObserver *observer)
{
    observers.push_back(std::shared_ptr<DevicesObserver>(observer));
}

void Devices::detach(DevicesObserver *observer)
{
    observers.erase(std::remove_if(observers.begin(), observers.end(), [observer](std::shared_ptr<DevicesObserver> o) { return o.get() == observer; }), observers.end());
}

void Devices::notify()
{
    for (auto observer : observers)
    {
        observer->update(devices);
    }
}