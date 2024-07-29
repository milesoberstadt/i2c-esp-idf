#if !defined(__DEVICES_OBSERVER__)
#define __DEVICES_OBSERVER__

#include <vector>

#include "types.h"

class DevicesObserver {
 public:
  virtual ~DevicesObserver(){};
  virtual void update(const std::vector<device_t> &devices) = 0;
};

#endif // __DEVICES_OBSERVER__


