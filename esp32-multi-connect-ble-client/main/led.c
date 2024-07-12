#include "led.h"

void init_led() {
    esp_err_t ret = gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(LED_TAG, "Failed to set LED pin direction: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(LED_TAG, "LED pin initialized");
}

void set_led(bool state) {
    gpio_set_level(LED_PIN, state);
}

bool get_led() {
    return gpio_get_level(LED_PIN);
}