#include "cache.h"

device_t cache[MAX_DEVICES];

void get_cache_device(device_t *dev, size_t dev_idx) {
    if (dev_idx < MAX_DEVICES) {
        *dev = cache[dev_idx];
    }
}

void set_cache_device_type(size_t idx, device_type_t type) {
    cache[idx].type = type;
}

void set_cache_device_state(size_t idx, device_state_t state) {
    cache[idx].state = state;
}

void set_cache_device_battery_level(size_t idx, uint8_t level) {
    cache[idx].battery_level = level;
}

void init_cache() {
    for (size_t i = 0; i < MAX_DEVICES; i++) {
        cache[i].type = UNKNOWN_DEVICE;
        cache[i].state = dev_state_disconnected;
        cache[i].battery_level = 0;
    }
}