#if !defined(__DEVICES_H__)
#define __DEVICES_H__

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_gattc_api.h"
#include "esp_bt.h"

#include "constants.h"
#include "preferences.h"

#define DEVICES_TAG "DEVICES"

#define DEVICE_KEY_PREFIX_SIZE 5   // support up to 99 devices (devXX)
#define DEVICE_KEY_SUFFIX_SIZE 4 
#define DEVICE_KEY_SIZE (DEVICE_KEY_PREFIX_SIZE + DEVICE_KEY_SUFFIX_SIZE)

#define DEVICE_COUNT_KEY "dev_count"

typedef struct device {
    esp_bd_addr_t bda;
    esp_ble_addr_type_t ble_addr_type;
} device;

bool init_devices();

void generate_device_key(size_t idx, char *tag, char* suffix);

bool device_exists(size_t idx);

bool add_device(esp_bd_addr_t bda, esp_ble_addr_type_t ble_addr_type, size_t idx);

bool remove_device(size_t idx);

size_t get_device_count();

void allocate_devices(device **devices, size_t count);

size_t get_devices(device *devices);

void free_devices(device *devices, size_t count);

#endif // __DEVICES_H__
