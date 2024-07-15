#include "ui.h"

size_t selected_device = 0;

size_t get_selected_device() {
    return selected_device;
}

void switch_selected_device() {
    selected_device = (selected_device + 1) % PROFILE_NUM;
    start_led_blink(selected_device, 3);
    ESP_LOGI(UI_TAG, "Selected device: %d", selected_device);
}