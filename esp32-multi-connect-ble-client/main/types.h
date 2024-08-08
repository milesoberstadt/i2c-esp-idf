#if !defined(__TYPES_H__)
#define __TYPES_H__

#include "driver/gpio.h"
#include "esp_gap_ble_api.h"
#include "freertos/FreeRTOS.h"

typedef struct {
    esp_bd_addr_t bda;
    esp_ble_addr_type_t ble_addr_type;
    size_t device_type;
} device_t;

typedef void (*button_callback)(void);

typedef struct {
    gpio_num_t gpio_num;
    button_callback press_callback;
    button_callback long_press_callback;
} button_config_t;

typedef struct {
    gpio_num_t gpio_num;
    button_callback press_callback;
    button_callback long_press_callback;
    TickType_t press_start_time;
    bool is_pressed;
} button_state_t;

typedef void (*data_callback_t)(size_t device_idx, size_t char_idx, uint8_t *value, uint16_t value_len);

typedef struct {
    esp_bt_uuid_t service_uuid;
    size_t char_count;
    esp_bt_uuid_t *char_uuids;
    data_callback_t data_callback;
} device_type_config_t;

typedef struct {
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    esp_bd_addr_t remote_bda;
    esp_ble_addr_type_t ble_addr_type;
    size_t device_type;
    bool connected;
    bool discovered;
    uint8_t subscribe_count;
    uint16_t *char_handles;
    data_callback_t data_callback;
} gattc_profile_inst;

typedef enum device_state_t {
    dev_state_error = -1,
    dev_state_disconnected = 0,
    dev_state_connected = 1,
    dev_state_pairing = 2,
    dev_state_disconnecting = 3,
    dev_state_connecting = 4,
} device_state_t;

typedef enum message_t {
    msg_err = 0,
    msg_init_start = 1,
    msg_init_end = 2,
    msg_dev_selected = 3,
    msg_dev_type = 10,
    msg_dev_state = 11,
    msg_dev_data = 12,
    msg_dev_error = 14,
    msg_screen_on = 50,
    msg_screen_off = 51,
} message_t;

typedef enum preference_t {
  PT_I8,
  PT_U8,
  PT_I16,
  PT_U16,
  PT_I32,
  PT_U32,
  PT_I64,
  PT_U64,
  PT_STR,
  PT_BLOB,
  PT_INVALID
} preference_t;

#endif // __TYPES_H__
