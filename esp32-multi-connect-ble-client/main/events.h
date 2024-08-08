#if !defined(__EVENTS_H__)
#define __EVENTS_H__

#include <stdio.h>
#include <stdbool.h>

#include "types.h"
#include "device_config.h"
#include "led.h"
#include "i2c_messages.h"
#include "display_controller.h"

#define EVENTS_TAG "EVENTS"

void on_screen_state_changed(bool is_screen_on);

void on_pairing_start(size_t dev_idx);

void on_pariring_stop(size_t dev_idx);

void on_device_selected(size_t dev_idx);

void on_device_type_changed(size_t dev_idx, device_type_t type);

void on_device_state_changed(size_t dev_idx, device_state_t state);

void on_data_received(size_t dev_idx, size_t char_idx, uint8_t *data, size_t len);

#endif // __EVENTS_H__
