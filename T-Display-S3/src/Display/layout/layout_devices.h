#if !defined(__LAYOUT_DEVICES__)
#define __LAYOUT_DEVICES__

#include "WString.h"

#include "layout.h"
#include "ui.h"
#include "types.h"

#include "devices_observer.h"
#include "devices.h"

#define LAYOUT_DEVICES_LINE_HEIGHT 30

class LayoutDevices : public Layout, public DevicesObserver
{
    public:
        LayoutDevices();
        ~LayoutDevices();
        void draw();

        void update(const std::vector<device_t> &devices);

    private:
        String device_type_str(device_type_t type);
        String device_state_str(device_state_t state);
        String device_value_str(uint8_t *value, uint8_t value_size);
};

#endif // __LAYOUT_DEVICES__
