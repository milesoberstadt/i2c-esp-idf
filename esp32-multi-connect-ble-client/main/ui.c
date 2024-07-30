#include "ui.h"

size_t selected_device = 0;
bool is_screen_on = true;

size_t get_selected_device() {
    return selected_device;
}

bool get_is_screen_on() {
    return is_screen_on;
}

void switch_selected_device() {
    selected_device = (selected_device + 1) % MAX_DEVICES;
    on_device_selected(selected_device);
}

void start_pairing() {
    if (get_is_scanning()) {
        ESP_LOGI(UI_TAG, "Already scanning");
        return;
    }

    if (is_profile_active(selected_device)) {
        disconnect(selected_device);
    }

    start_scan();

}

void reconnect_device() {
    if (is_profile_active(selected_device)) {
        ESP_LOGI(UI_TAG, "Can't reconnect %d, connection already active or in progress.", selected_device);
        return;
    }
    connect_device(selected_device);
}

void toggle_screen() {
    is_screen_on = !is_screen_on;
    on_screen_state_changed(is_screen_on);
}

bool init_ui() {

    bool ret = init_led();
    if (!ret) {
        ESP_LOGE(UI_TAG, "Failed to initialize LED");
        return false;
    }

    // PAIR BUTTON

    ESP_LOGI(UI_TAG, "Initializing pair button");
    button_config_t pair_button_config = {
        .gpio_num = PAIR_BUTTON_PIN,
        .press_callback = reconnect_device,
        .long_press_callback = start_pairing
    };

    ret = init_button(&pair_button_config);
    if (!ret) {
        ESP_LOGE(UI_TAG, "Failed to initialize pair button");
        return false;
    }
    ESP_LOGI(UI_TAG, "Pair button initialized");

    // SELECT BUTTON

    ESP_LOGI(UI_TAG, "Initializing select button");
    button_config_t select_button_config = {
        .gpio_num = SELECT_BUTTON_PIN,
        .press_callback = switch_selected_device,
    };
    ret = init_button(&select_button_config);
    if (!ret) {
        ESP_LOGE(UI_TAG, "Failed to initialize select button");
        return false;
    }
    ESP_LOGI(UI_TAG, "Select button initialized");

    // SCREEN BUTTON

    ESP_LOGI(UI_TAG, "Initializing screen button");
    button_config_t screen_button_config = {
        .gpio_num = SCREEN_BUTTON_PIN,
        .press_callback = toggle_screen,
        .long_press_callback = NULL
    };

    ret = init_button(&screen_button_config);
    if (!ret) {
        ESP_LOGE(UI_TAG, "Failed to initialize screen button");
        return false;
    }
    ESP_LOGI(UI_TAG, "Screen button initialized");


    return true;
    
}