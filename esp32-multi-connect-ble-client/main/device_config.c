#include "device_config.h"

static device_type_config_t DEVICE_A_NODE_CONFIG;
static device_type_config_t DEVICE_M_NODE_CONFIG;
static device_type_config_t DEVICE_C_NODE_CONFIG;

static device_type_config_t DEVICE_CONFIGS[DEVICE_TYPE_COUNT];

bool init_device_config() {

    // init M-Node device config
    DEVICE_M_NODE_CONFIG.service_uuid = UUID_M_NODE;
    DEVICE_M_NODE_CONFIG.char_count = 1;
    DEVICE_M_NODE_CONFIG.char_uuids = (esp_bt_uuid_t[]) {UUID_GYRO_CHAR};
    DEVICE_M_NODE_CONFIG.data_callback = gyro_data_callback;
    DEVICE_CONFIGS[DEVICE_M_NODE] = DEVICE_M_NODE_CONFIG;

    // todo : init A-Node device config

    // todo : init Slipper device config

    return true;

}

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