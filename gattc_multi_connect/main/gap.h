#if !defined(__GAP_H__)
#define __GAP_H__

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_gap_ble_api.h"

#include "constants.h"

void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

void start_scan(void);

#endif // __GAP_H__
