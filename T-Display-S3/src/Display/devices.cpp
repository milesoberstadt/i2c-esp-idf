#include "devices.h"

Devices::Devices()
{
    devices = std::vector<device_t>(DEVICE_COUNT);
}

Devices::~Devices()
{
    devices.clear();
}

void Devices::setDeviceState(uint8_t idx, device_state_t state)
{
    devices[idx].state = state;
}

void Devices::setDeviceValue(uint8_t idx, uint8_t* value, uint8_t value_size)
{
    devices[idx].value = value;
    devices[idx].value_size = value_size;
}

device_t Devices::getDevice(uint8_t idx)
{
    return devices[idx];
}

std::vector<device_t> Devices::getDevices()
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