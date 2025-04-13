#if !defined(__CACHE_H__)
#define __CACHE_H__

#include "types.h"
#include "device_config.h"

void init_cache();

void get_cache_device(device_t *dev, size_t dev_idx);

void set_cache_device_type(size_t idx, device_type_t type);

void set_cache_device_state(size_t idx, device_state_t state);

void set_cache_device_battery_level(size_t idx, uint8_t level);


#endif // __CACHE_H__
