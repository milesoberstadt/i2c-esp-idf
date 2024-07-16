#include "ui.h"

size_t selected_device = 0;

size_t get_selected_device() {
    return selected_device;
}

void switch_selected_device() {
    selected_device = (selected_device + 1) % MAX_DEVICES;
    start_led_blink(selected_device, 1);
    ESP_LOGI(UI_TAG, "Selected device: %d", selected_device);
}

void start_pairing() {
    if (get_is_scanning()) {
        ESP_LOGI(UI_TAG, "Already scanning");
        return;
    }

    start_scan();

}

bool init_ui() {

    bool ret = init_led();
    if (!ret) {
        ESP_LOGE(UI_TAG, "Failed to initialize LED");
        return false;
    }

    struct button_config_t button_config = {
        .gpio_num = BUTTON_PIN,
        .press_callback = switch_selected_device,
        .long_press_callback = start_pairing
    };

    ret = init_button(&button_config);
    if (!ret) {
        ESP_LOGE(UI_TAG, "Failed to initialize button");
        return false;
    }

    return true;
    
}