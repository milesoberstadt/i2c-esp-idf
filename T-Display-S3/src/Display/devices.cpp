#include "devices.hpp"

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