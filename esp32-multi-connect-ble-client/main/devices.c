#include "devices.h"

void generate_device_key(size_t idx, char *tag, char* suffix) {
    snprintf(tag, DEVICE_KEY_SIZE+DEVICE_KEY_SUFFIX_SIZE, "dev%02d%s", idx, suffix);
}

bool init_devices() {
    return init_preferences();
}

bool device_exists(size_t idx) {
    char addr_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, addr_key, DEVICE_BLE_ADDR_KEY);
    return is_preference_key(addr_key);
}

bool add_device(esp_bd_addr_t bda, esp_ble_addr_type_t ble_addr_type, size_t device_type, size_t idx) {

    char addr_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, addr_key, DEVICE_BLE_ADDR_KEY);

    if (is_preference_key(addr_key)) {
    
        device_t dev;
        bool ret = get_device(idx, &dev);
        if (!ret) {
            ESP_LOGE(DEVICES_TAG, "Can't read existing device at idx %d. Can't override.", idx);
            return false;
        }

        if (memcmp(bda, dev.bda, 6) == 0) {
            ESP_LOGI(DEVICES_TAG,  "Device already saved at idx %d. Skipping device save.", idx);
            return true;
        }
        
        ESP_LOGI(DEVICES_TAG, "A different device already exist at idx %d, overriding ...", idx);
        ret = remove_device(idx);
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
    generate_device_key(idx, type_key, DEVICE_BLE_ADDR_TYPE_KEY);
    ret = put_int(type_key, ble_addr_type);
    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error adding device idx %d (type)", idx);
        return false;
    }

    char device_type_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, device_type_key, DEVICE_TYPE_KEY);
    ret = put_int(device_type_key, device_type);
    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error adding device idx %d (device type)", idx);
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
    generate_device_key(idx, addr_key, DEVICE_BLE_ADDR_KEY);
    if (!is_preference_key(addr_key)) {
        ESP_LOGE(DEVICES_TAG, "Can't remove device idx %d, device doesn't exist", idx);
        return false;
    }

    bool ret = remove_preference(addr_key);
    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error removing device idx %d (address)", idx);
        return true;
    }

    char type_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, type_key, DEVICE_BLE_ADDR_TYPE_KEY);
    ret = remove_preference(type_key);
    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error removing device idx %d (type)", idx);
    }

    char device_type_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, device_type_key, DEVICE_TYPE_KEY);
    ret = remove_preference(device_type_key);
    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error removing device idx %d (device type)", idx);
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

bool get_device(size_t idx, device_t *dev) {

    char addr_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, addr_key, DEVICE_BLE_ADDR_KEY);
    size_t len = get_bytes_length(addr_key);
    if (!len) {
        ESP_LOGE(DEVICES_TAG, "Error reading device idx %d (address)", idx);
        return false;
    }
    esp_bd_addr_t bda;
    size_t res = get_bytes(addr_key, bda, len);
    if (!res) {
        ESP_LOGE(DEVICES_TAG, "Error reading device idx %d (address)", idx);
        return false;
    }

    char type_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, type_key, DEVICE_BLE_ADDR_TYPE_KEY);
    int32_t ble_addr_type = get_int(type_key, -1);
    if (ble_addr_type == -1) {
        ESP_LOGE(DEVICES_TAG, "Error reading device idx %d (type)", idx);
        return false;
    }

    char device_type_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, device_type_key, DEVICE_TYPE_KEY);
    int32_t device_type = get_int(device_type_key, -1);
    if (device_type == -1) {
        ESP_LOGE(DEVICES_TAG, "Error reading device idx %d (device type)", idx);
        return false;
    }

    memcpy(dev->bda, bda, sizeof(esp_bd_addr_t));
    dev->ble_addr_type = ble_addr_type;
    dev->device_type = device_type;

    return true;

}

void connect_device(size_t idx) {

    device_t dev;
    bool ret = get_device(idx, &dev);

    if (!ret) {
        return;
    }

    ESP_LOGI(DEVICES_TAG, "Connecting to device idx %d", idx);
    
    open_profile(dev.bda, dev.ble_addr_type, idx, dev.device_type);

}

void connect_all_devices() {

    size_t dev_count = get_device_count();
    if (!dev_count) {
        ESP_LOGE(DEVICES_TAG, "No devices to connect to.");
        return;
    }

    for (size_t i = 0; i < dev_count; i++) {
        connect_device(i);
    }

}

bool update_device_bda(size_t idx, esp_bd_addr_t bda) {
    
        device_t dev;
        bool ret = get_device(idx, &dev);
        if (!ret) {
            return false;
        }
    
        if (memcmp(bda, dev.bda, 6) == 0) {
            ESP_LOGI(DEVICES_TAG, "Device idx %d already has the same address. Skipping update.", idx);
            return true;
        }
    
        char addr_key[DEVICE_KEY_SIZE];
        generate_device_key(idx, addr_key, DEVICE_BLE_ADDR_KEY);
        size_t res = put_bytes(addr_key, bda, sizeof(esp_bd_addr_t));
        if (!res) {
            ESP_LOGE(DEVICES_TAG, "Error updating device idx %d (address)", idx);
            return false;
        }
    
        ESP_LOGI(DEVICES_TAG, "Updated bda for device idx %d", idx);
    
        return true;
}