#include "button.h"

static esp_timer_handle_t debounce_timer;

// debounced callback
void IRAM_ATTR button_start_scan(void* arg) {
      int button_state = gpio_get_level(BUTTON_PIN);

    if (button_state == 1)  // button is released
    {
        return;
    }

    if (!get_is_scanning()) {
        start_scan();
    }
}

// the callback on button press
void IRAM_ATTR button_handler(void* arg) {
    esp_timer_stop(debounce_timer);
    esp_timer_start_once(debounce_timer, DEBOUNCE_TIME_MS * 1000);
}

void init_button(gpio_isr_t isr_handler) {
    esp_err_t ret = gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Failed to set button pin direction: %s", esp_err_to_name(ret));
        return;
    }

    ret = gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);  // Enable internal pull-up resistor
    if (ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Failed to set button pin pull mode: %s", esp_err_to_name(ret));
        return;
    }

    ret = gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_NEGEDGE);  // Trigger on falling edge (button press)
    if (ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Failed to set button pin interrupt type: %s", esp_err_to_name(ret));
        return;
    }

    ret = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);    // Install the default ISR service
    if (ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Failed to install ISR service: %s", esp_err_to_name(ret));
        return;
    }

    ret = gpio_isr_handler_add(BUTTON_PIN, button_handler, (void*) BUTTON_PIN);
    if (ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Failed to add ISR handler: %s", esp_err_to_name(ret));
        return;
    }

    const esp_timer_create_args_t debounce_timer_args = {
        .callback = isr_handler,
        .name = "debounce_timer"
    };
    ret = esp_timer_create(&debounce_timer_args, &debounce_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Failed to create debounce timer: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(BUTTON_TAG, "Button pin initialized");

}