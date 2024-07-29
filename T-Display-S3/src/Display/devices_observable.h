#if !defined(__DEVICES_OBSERVABLE__)
#define __DEVICES_OBSERVABLE__

#include "devices_observer.h"
#include "types.h"

class DevicesObservable {
 public:
  virtual void attach(DevicesObserver *observer) = 0;
  virtual void detach(DevicesObserver *observer) = 0;
  virtual void notify() = 0;
};

#endif // __DEVICES_OBSERVABLE__
