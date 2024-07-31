#include "display_controller.h"

bool init_display() {

    esp_err_t ret = gpio_set_direction(DISPLAY_WAKEUP_PIN, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(DISPLAY_TAG, "Failed to set display wakeup pin direction: %s", esp_err_to_name(ret));
        return false;
    }

    return true;

}

void display_sleep() {
    i2c_send_message(msg_screen_off, 0);
}

void display_wake() {
    gpio_set_level(DISPLAY_WAKEUP_PIN, 1);
    i2c_send_message(msg_screen_on, 0);
    gpio_set_level(DISPLAY_WAKEUP_PIN, 0);
}