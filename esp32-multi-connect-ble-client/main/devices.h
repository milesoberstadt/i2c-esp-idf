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
#include "gattc.h"
#include "device_config.h"
#include "types.h"

#define DEVICES_TAG "DEVICES"

#define DEVICE_COUNT_KEY "dev_count"

/* Device keys configuration */
#define DEVICE_KEY_PREFIX_SIZE 5   // support up to 99 devices (devXX)
#define DEVICE_KEY_SUFFIX_SIZE 4 
#define DEVICE_KEY_SIZE (DEVICE_KEY_PREFIX_SIZE + DEVICE_KEY_SUFFIX_SIZE)

/* Device key names */
/* these keys should be DEVICE_KEY_SUFFIX_SIZE long */
#define DEVICE_BLE_ADDR_KEY "addr"
#define DEVICE_BLE_ADDR_TYPE_KEY "bl_t"
#define DEVICE_TYPE_KEY "dv_t"

bool init_devices();

void generate_device_key(size_t idx, char *tag, char* suffix);

bool device_exists(size_t idx);

bool add_device(esp_bd_addr_t bda, esp_ble_addr_type_t ble_addr_type, size_t device_type, size_t idx);

bool remove_device(size_t idx);

size_t get_device_count();

bool get_device(size_t idx, device_t *dev);

void connect_device(size_t idx);

void connect_all_devices();

bool update_device_bda(size_t idx, esp_bd_addr_t bda);

#endif // __DEVICES_H__
