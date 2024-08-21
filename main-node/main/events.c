#include "events.h"

void on_screen_state_changed(bool is_screen_on) {
    #if LOG_EVENTS == 1
    ESP_LOGI(EVENTS_TAG, "Screen %s", is_screen_on ? "on" : "off");
    #endif
    i2c_send_message(msg_screen_toggle,0);
}

void on_pairing_start(size_t dev_idx) {
    #if LOG_EVENTS == 1
    ESP_LOGI(EVENTS_TAG, "Pairing started for device %d", dev_idx);
    #endif
    start_led_blink(dev_idx, -1, 500);
    set_cache_device_state(dev_idx, dev_state_pairing);
    i2c_send_message_data(  msg_dev_state,
                            dev_idx,  
                            (uint8_t *)dev_state_pairing, 
                            sizeof(device_state_t));
}

void on_pariring_stop(size_t dev_idx) {
    #if LOG_EVENTS == 1
    ESP_LOGI(EVENTS_TAG, "Pairing stopped for device %d", dev_idx);
    #endif
    stop_led_blink(dev_idx);
    set_cache_device_state(dev_idx, dev_state_disconnected);
    i2c_send_message_data(  msg_dev_state, 
                            dev_idx, 
                            (uint8_t *)dev_state_disconnected, 
                            sizeof(device_state_t));
}

void on_device_selected(size_t dev_idx) {
    #if LOG_EVENTS == 1
    ESP_LOGI(EVENTS_TAG, "Device %d selected", dev_idx);
    #endif
    start_led_blink(dev_idx, 1, 100);
    i2c_send_message(msg_dev_selected, dev_idx);
}

void on_device_type_changed(size_t dev_idx, device_type_t type) {
    #if LOG_EVENTS == 1
    ESP_LOGI(EVENTS_TAG, "Device %d type changed to %d", dev_idx, type);
    #endif
    set_cache_device_type(dev_idx, type);
    i2c_send_message_data(  msg_dev_type, 
                            dev_idx, 
                            (uint8_t *)&type, 
                            sizeof(device_type_t));
}

void on_device_state_changed(size_t dev_idx, device_state_t state) {
    #if LOG_EVENTS == 1
    ESP_LOGI(EVENTS_TAG, "Device %d state changed to %d", dev_idx, state);
    #endif
    set_cache_device_state(dev_idx, state);
    i2c_send_message_data(  msg_dev_state,
                            dev_idx, 
                            (uint8_t *)&state, 
                            sizeof(uint8_t));

    switch (state)
    {
    case dev_state_pairing:
        start_led_blink(dev_idx, -1, 300);
        break;
    case dev_state_connecting:
        start_led_blink(dev_idx, -1, 100);
        break;
    case dev_state_connected:
        stop_led_blink(dev_idx);
        set_led(dev_idx, true);
        break;
    case dev_state_disconnecting:
        start_led_blink(dev_idx, -1, 100);
        break;
    case dev_state_disconnected:
        reset_led(dev_idx);
        break;
    default:
        break;
    }
}

void on_data_received(size_t dev_idx, size_t char_idx, uint8_t *data, size_t len) {
    // ESP_LOGI(EVENTS_TAG, "Data received from device %d, char %d: %s", dev_idx, char_idx, data);
    i2c_send_message_data(  msg_dev_data,
                            dev_idx, 
                            data, 
                            len);
}

void on_battery_level_received(size_t dev_idx, uint8_t level) {
    #if LOG_EVENTS == 1
    ESP_LOGI(EVENTS_TAG, "Battery level received from device %d: %d", dev_idx, level);
    #endif
    set_cache_device_battery_level(dev_idx, level);
    i2c_send_message_data(  msg_dev_battery_level,
                            dev_idx,
                            &level, 
                            sizeof(uint8_t));
}