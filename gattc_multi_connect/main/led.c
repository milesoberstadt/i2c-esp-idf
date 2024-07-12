#include "led.h"

void init_led() {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

void set_led(bool state) {
    gpio_set_level(LED_GPIO, state);
}

bool get_led() {
    return gpio_get_level(LED_PIN);
}