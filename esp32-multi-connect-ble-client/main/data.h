#if !defined(__DATA_H__)
#define __DATA_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "esp_log.h"

#include "types.h"

#define DATA_TAG "DATA"

/** --- Define your device data callbacks here --- */

void m_node_cb(size_t device_idx, uint8_t *value, uint16_t value_len);

void a_node_cb(size_t device_idx, uint8_t *value, uint16_t value_len);

#endif // __DATA_H__
