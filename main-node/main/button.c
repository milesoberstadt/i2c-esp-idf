#include "button.h"

static button_state_t *button_states;
static int num_buttons;

static void button_task(void *param) {
    while (1) {
        for (int i = 0; i < num_buttons; i++) {
            button_state_t *button = &button_states[i];
            bool is_button_pressed = gpio_get_level(button->gpio_num) == 0;

            if (is_button_pressed && !button->is_pressed) {
                button->is_pressed = true;
                button->press_start_time = xTaskGetTickCount();
            } else if (!is_button_pressed && button->is_pressed) {
                button->is_pressed = false;
                TickType_t press_duration = xTaskGetTickCount() - button->press_start_time;

                if (press_duration >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME)) {
                    if (press_duration >= pdMS_TO_TICKS(1000)) {
                        if (button->long_press_callback) {
                            button->long_press_callback();
                        }
                    } else {
                        if (button->press_callback) {
                            button->press_callback();
                        }
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool init_button(button_config_t *config) {

    ESP_LOGI(BUTTON_TAG, "Initializing button");

    if (button_states == NULL) {
        button_states = malloc(sizeof(button_state_t));
    } else {
        button_state_t *new_button_states = realloc(button_states, sizeof(button_state_t) * (num_buttons + 1));

        if (new_button_states == NULL) {
            ESP_LOGE(BUTTON_TAG, "Failed to allocate memory for button states");
            return false;
        }

        button_states = new_button_states;
    }

    button_state_t *button = &button_states[num_buttons];
    button->gpio_num = config->gpio_num;
    button->press_callback = config->press_callback;
    button->long_press_callback = config->long_press_callback;
    button->press_start_time = 0;
    button->is_pressed = false;

    gpio_set_direction(button->gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(button->gpio_num, GPIO_PULLUP_ONLY);

    if (num_buttons == 0) {
        xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
    }

    num_buttons++;

    ESP_LOGI(BUTTON_TAG, "Button initialized");

    return true;
}
