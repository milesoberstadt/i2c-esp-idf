#if !defined(__LAYOUT_DEVICES__)
#define __LAYOUT_DEVICES__

#include "layout.h"

#include "devices_observer.h"
#include "devices.h"

class LayoutDevices : public Layout, public DevicesObserver
{
    public:
        LayoutDevices();
        ~LayoutDevices();
        void draw();

        void update(const std::vector<device_t> &devices);
};

#endif // __LAYOUT_DEVICES__
