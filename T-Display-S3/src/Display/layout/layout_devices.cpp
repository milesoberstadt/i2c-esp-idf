#include "layout_devices.h"

LayoutDevices::LayoutDevices()
{
}

LayoutDevices::~LayoutDevices()
{
}

void LayoutDevices::draw()
{

    Devices &devices = Devices::getInstance();

    std::vector<device_t> devices_list = devices.getDevices();

    this->update(devices_list);

}

void LayoutDevices::update(const std::vector<device_t> &devices)
{
}