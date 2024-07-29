#if !defined(__LAYOUT_DEVICES__)
#define __LAYOUT_DEVICES__

#include "WString.h"

#include "layout.h"
#include "ui.h"
#include "types.h"

#include "devices_observer.h"
#include "devices.h"
#include "utils.h"

#define LAYOUT_DEVICES_LINE_HEIGHT 30

class LayoutDevices : public Layout, public DevicesObserver
{
    public:
        LayoutDevices();
        ~LayoutDevices();
        void draw();

        void update(const std::vector<device_t> &devices);
};

#endif // __LAYOUT_DEVICES__
