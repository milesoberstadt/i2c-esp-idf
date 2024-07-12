#include "led.h"

static SemaphoreHandle_t xSemaphore = NULL;
static bool led_blinking = false;
static int blink_delay = 500;

void led_blink_task(void *pvParameter)
{

    while (1)
    {
        // Wait for the semaphore
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Toggle the LED based on the led_blinking state
            while (led_blinking)
            {
                gpio_set_level(LED_PIN, 1);
                vTaskDelay(blink_delay / portTICK_PERIOD_MS);  // LED on for 500ms

                gpio_set_level(LED_PIN, 0);
                vTaskDelay(blink_delay / portTICK_PERIOD_MS);  // LED off for 500ms

                // Check if the led_blinking state has changed
                if (xSemaphoreTake(xSemaphore, 0) == pdTRUE)
                {
                    break;
                }
            }
        }
    }
}

void init_led() {
    esp_err_t ret = gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(LED_TAG, "Failed to set LED pin direction: %s", esp_err_to_name(ret));
        return;
    }

    xSemaphore = xSemaphoreCreateBinary();

    if (xSemaphore == NULL)
    {
        ESP_LOGE(LED_TAG, "Failed to create semaphore");
        return;
    }

    int result = xTaskCreate(&led_blink_task, "led_blink_task", 2048, NULL, 5, NULL);
    if (result != pdPASS)
    {
        ESP_LOGE(LED_TAG, "Failed to create led_blink_task");
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

void start_led_blink(int delay_ms) {
    blink_delay = delay_ms;
    led_blinking = true;
    xSemaphoreGiveFromISR(xSemaphore, NULL);
}

void stop_led_blink() {
    led_blinking = false;
    xSemaphoreGiveFromISR(xSemaphore, NULL);
}