#include "devices.hpp"

Devices::Devices()
{
}

Devices::~Devices()
{
}

void Devices::addDevice(device_t device)
{
    devices.push_back(device);
}

void Devices::removeDevice(device_t device)
{
    for (std::vector<device_t>::iterator it = devices.begin(); it != devices.end(); ++it)
    {
        if (it->type == device.type)
        {
            devices.erase(it);
            break;
        }
    }
}

void Devices::updateDevice(device_t device)
{
    for (std::vector<device_t>::iterator it = devices.begin(); it != devices.end(); ++it)
    {
        if (it->type == device.type)
        {
            it->state = device.state;
            it->value = device.value;
            it->value_size = device.value_size;
            break;
        }
    }
}

void Devices::updateDeviceState(device_t device, device_state_t state)
{
    for (std::vector<device_t>::iterator it = devices.begin(); it != devices.end(); ++it)
    {
        if (it->type == device.type)
        {
            it->state = state;
            break;
        }
    }
}

void Devices::updateDeviceValue(device_t device, uint8_t* value, uint8_t value_size)
{
    for (std::vector<device_t>::iterator it = devices.begin(); it != devices.end(); ++it)
    {
        if (it->type == device.type)
        {
            it->value = value;
            it->value_size = value_size;
            break;
        }
    }
}

device_t Devices::getDevice(device_type_t type)
{
    for (std::vector<device_t>::iterator it = devices.begin(); it != devices.end(); ++it)
    {
        if (it->type == type)
        {
            return *it;
        }
    }
    device_t device;
    device.type = UNKNOWN_DEVICE;
    device.state = error;
    device.value = NULL;
    device.value_size = 0;
    return device;
}

std::vector<device_t> Devices::getDevices()
{
    return devices;
}