#include "layout_devices.h"

LayoutDevices::LayoutDevices()
{

    UI &ui = UI::instance();

    ui.background();
    ui.titleStyle();
    ui.getGFX()->print("Devices");

}

LayoutDevices::~LayoutDevices()
{
}

void LayoutDevices::draw()
{

    Devices &devices = Devices::instance();

    std::vector<device_t> devices_list = devices.get_devices();

    this->update(devices_list);

}

void LayoutDevices::update(const std::vector<device_t> &devices)
{

    UI &ui = UI::instance();

    ui.background(0, 0, ui.width() - 30, ui.height());
    ui.getGFX()->setCursor(0, 0);

    size_t i = 0;
    for (auto &device : devices)
    {

        size_t cursor_y = i*LAYOUT_DEVICES_LINE_HEIGHT;

        ui.getGFX()->setCursor(0, cursor_y);
        ui.textStyle();
        ui.getGFX()->print(i++);
        ui.getGFX()->print(". ");

        switch (device.state)
        {
            case dev_state_connected:
                ui.getGFX()->print(device_type_str(device.type));

                ui.smallTextStyle();
                ui.getGFX()->setCursor(0, 17 + cursor_y);
                ui.getGFX()->print(device_value_str(device.value, device.value_size));

                break;
            default:
                ui.getGFX()->print(device_state_str(device.state));
                break;
        }


    }

}