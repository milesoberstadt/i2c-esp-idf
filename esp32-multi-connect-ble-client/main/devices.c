#include "devices.h"

void generate_device_key(size_t idx, char *tag, char* suffix) {
    snprintf(tag, DEVICE_KEY_SIZE+DEVICE_KEY_SUFFIX_SIZE, "dev%02d%s", idx, suffix);
}

bool init_devices() {
    return init_preferences();
}

bool device_exists(size_t idx) {
    char addr_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, addr_key, "addr");
    return isKey(addr_key);
}

bool add_device(esp_bd_addr_t bda, esp_ble_addr_type_t ble_addr_type, size_t idx) {

    char addr_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, addr_key, "addr");
    if (isKey(addr_key)) {
        ESP_LOGE(DEVICES_TAG, "Device %d already exist, overriding ...", idx);
        bool ret = remove_device(idx);
        if (!ret) {
            ESP_LOGE(DEVICES_TAG, "Error removing device idx %d. Can't override.", idx);
            return false;
        }
    }

    size_t ret = put_bytes(addr_key, bda, sizeof(esp_bd_addr_t));
    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error adding device idx %d (address)", idx);
        return false;
    }

    char type_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, type_key, "type");
    ret = put_int(type_key, ble_addr_type);
    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error adding device idx %d (type)", idx);
        return false;
    }

    size_t dev_count = get_device_count();
    ret = put_int(DEVICE_COUNT_KEY, dev_count + 1);
    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error adding device idx %d (incrementing counter)", idx);
        return false;
    }

    ESP_LOGI(DEVICES_TAG, "Saved device idx %d", idx);

    return true;

}

bool remove_device(size_t idx) {

    char addr_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, addr_key, "addr");
    if (!isKey(addr_key)) {
        ESP_LOGE(DEVICES_TAG, "Can't remove device idx %d, device doesn't exist", idx);
        return false;
    }

    bool ret = remove_preference(addr_key);
    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error removing device idx %d (address)", idx);
        return true;
    }

    char type_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, type_key, "type");
    ret = remove_preference(type_key);
    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error removing device idx %d (type)", idx);
    }

    size_t dev_count = get_device_count();
    size_t res = put_int(DEVICE_COUNT_KEY, dev_count - 1);
    if (!res) {
        ESP_LOGE(DEVICES_TAG, "Error removing device idx %d (decrementing counter)", idx);
    }

    ESP_LOGI(DEVICES_TAG, "Removed device idx %d", idx);

    return true;

}

size_t get_device_count() {
    return get_int(DEVICE_COUNT_KEY, 0);
}

size_t get_devices(device *devices) {

    size_t dev_count = get_device_count();
    if (!dev_count) {
        return 0;
    }

    for (size_t i = 0; i < dev_count; i++) {
        char addr_key[DEVICE_KEY_SIZE];
        generate_device_key(i, addr_key, "addr");
        size_t len = get_bytes_length(addr_key);
        if (!len) {
            ESP_LOGE(DEVICES_TAG, "Error reading device idx %d (address)", i);
            return 0;
        }
        esp_bd_addr_t bda;
        size_t res = get_bytes(addr_key, bda, len);
        if (!res) {
            ESP_LOGE(DEVICES_TAG, "Error reading device idx %d (address)", i);
            return 0;
        }

        char type_key[DEVICE_KEY_SIZE];
        generate_device_key(i, type_key, "type");
        int32_t ble_addr_type = get_int(type_key, -1);
        if (ble_addr_type == -1) {
            ESP_LOGE(DEVICES_TAG, "Error reading device idx %d (type)", i);
            return 0;
        }

        memcpy(devices[i].bda, bda, sizeof(esp_bd_addr_t));
        devices[i].ble_addr_type = ble_addr_type;
    }

    return dev_count;

}

void allocate_devices(device **devices, size_t count) {
    *devices = (device *)malloc(count * sizeof(device));
    if (*devices == NULL) {
        ESP_LOGE(DEVICES_TAG, "Error allocating memory for devices array.");
        return;
    }

    ESP_LOGI(DEVICES_TAG, "Allocated memory for devices array.");
}

void free_devices(device *devices, size_t count) {
    free(devices);
    ESP_LOGI(DEVICES_TAG, "Freed memory for devices array.");
}

