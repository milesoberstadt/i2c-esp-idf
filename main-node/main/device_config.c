#include "device_config.h"

device_type_config_t get_device_config(device_type_t type) {
    if (type == UNKNOWN_DEVICE) {
        ESP_LOGE(DEVICE_CONFIG_TAG, "Unknown device type");
        return DEVICE_CONFIGS[UNKNOWN_DEVICE];
    }

    return DEVICE_CONFIGS[type];
}

device_type_t get_device_type_from_uuid(uint8_t *service_uuid) {
    for (size_t i = 0; i < DEVICE_TYPE_COUNT; i++) {
        if (compare_uuid(service_uuid, DEVICE_CONFIGS[i].service_uuid.uuid.uuid128)) {
            return i;
        }
    }

    return UNKNOWN_DEVICE;
}