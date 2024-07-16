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
    
        device dev;
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

bool get_device(size_t idx, device *dev) {

    char addr_key[DEVICE_KEY_SIZE];
    generate_device_key(idx, addr_key, "addr");
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
    generate_device_key(idx, type_key, "type");
    int32_t ble_addr_type = get_int(type_key, -1);
    if (ble_addr_type == -1) {
        ESP_LOGE(DEVICES_TAG, "Error reading device idx %d (type)", idx);
        return false;
    }

    memcpy(dev->bda, bda, sizeof(esp_bd_addr_t));
    dev->ble_addr_type = ble_addr_type;

    return true;

}

// size_t get_devices(device *devices) {

//     size_t dev_count = get_device_count();
//     if (!dev_count) {
//         return 0;
//     }

//     for (size_t i = 0; i < dev_count; i++) {
//         device dev;
//         bool res = get_device(i, &dev);
//         if (!res) {
//             ESP_LOGE(DEVICES_TAG, "Error reading device idx %d", i);
//             return 0;
//         }
//         devices[i] = dev;
//     }

//     return dev_count;

// }

// void allocate_devices(device **devices, size_t count) {
//     *devices = (device *)malloc(count * sizeof(device));
//     if (*devices == NULL) {
//         ESP_LOGE(DEVICES_TAG, "Error allocating memory for devices array.");
//         return;
//     }

//     ESP_LOGI(DEVICES_TAG, "Allocated memory for devices array.");
// }

// void free_devices(device *devices, size_t count) {
//     free(devices);
//     ESP_LOGI(DEVICES_TAG, "Freed memory for devices array.");
// }

void connect_device(size_t idx) {

    device dev;
    bool ret = get_device(idx, &dev);

    if (!ret) {
        ESP_LOGE(DEVICES_TAG, "Error reading device idx %d", idx);
        return;
    }

    ESP_LOGI(DEVICES_TAG, "Connecting to device idx %d", idx);
    
    open_profile(dev.bda, dev.ble_addr_type, idx);

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
    
        device dev;
        bool ret = get_device(idx, &dev);
        if (!ret) {
            ESP_LOGE(DEVICES_TAG, "Error reading device idx %d", idx);
            return false;
        }
    
        if (memcmp(bda, dev.bda, 6) == 0) {
            ESP_LOGI(DEVICES_TAG, "Device idx %d already has the same address. Skipping update.", idx);
            return true;
        }
    
        char addr_key[DEVICE_KEY_SIZE];
        generate_device_key(idx, addr_key, "addr");
        size_t res = put_bytes(addr_key, bda, sizeof(esp_bd_addr_t));
        if (!res) {
            ESP_LOGE(DEVICES_TAG, "Error updating device idx %d (address)", idx);
            return false;
        }
    
        ESP_LOGI(DEVICES_TAG, "Updated bda for device idx %d", idx);
    
        return true;
}