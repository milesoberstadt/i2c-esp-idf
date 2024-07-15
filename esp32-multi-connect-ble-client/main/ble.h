#if !defined(__BLE_H__)
#define __BLE_H__

#include <stdbool.h>
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_log.h"

#define BLE_TAG "BLE"

bool init_ble();

#endif // __BLE_H__
