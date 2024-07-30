#include "events.h"

void on_pairing_start(size_t dev_idx) {
    ESP_LOGI(EVENTS_TAG, "Pairing started for device %d", dev_idx);
    start_led_blink(dev_idx, -1, 500);
    i2c_send_message_data(  dev_idx, 
                            msg_dev_state, 
                            (uint8_t *)dev_state_pairing, 
                            sizeof(device_state_t));
}

void on_pariring_stop(size_t dev_idx) {
    ESP_LOGI(EVENTS_TAG, "Pairing stopped for device %d", dev_idx);
    stop_led_blink(dev_idx);
    i2c_send_message_data(  dev_idx, 
                            msg_dev_state, 
                            (uint8_t *)dev_state_disconnected, 
                            sizeof(device_state_t));
}

void on_device_selected(size_t dev_idx) {
    ESP_LOGI(EVENTS_TAG, "Device %d selected", dev_idx);
    start_led_blink(dev_idx, 1, 100);
    i2c_send_message(   dev_idx, 
                        msg_dev_selected);
}

void on_device_type_changed(size_t dev_idx, device_type_t type) {
    ESP_LOGI(EVENTS_TAG, "Device %d type changed to %d", dev_idx, type);
    i2c_send_message_data(  dev_idx, 
                            msg_dev_type, 
                            (uint8_t *)&type, 
                            sizeof(device_type_t));
}

void on_device_state_changed(size_t dev_idx, device_state_t state) {
    ESP_LOGI(EVENTS_TAG, "Device %d state changed to %d", dev_idx, state);

    i2c_send_message_data(  dev_idx, 
                            msg_dev_state,
                            (uint8_t *)&state, 
                            sizeof(device_state_t));

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
        stop_led_blink(dev_idx);
        set_led(dev_idx, false);
        break;
    default:
        break;
    }
}

void on_data_received(size_t dev_idx, size_t char_idx, uint8_t *data, size_t len) {
    ESP_LOGI(EVENTS_TAG, "Data received from device %d, char %d: %s", dev_idx, char_idx, data);
    i2c_send_message_data(  dev_idx, 
                            msg_dev_data,
                            data, 
                            len);
}