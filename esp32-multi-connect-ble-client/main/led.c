#include "led.h"

static SemaphoreHandle_t xSemaphore[MAX_DEVICES] = {NULL}; // Semaphore array for each LED
static bool led_states[MAX_DEVICES] = {0}; // State array for each LED
static int led_blinking[MAX_DEVICES] = {0}; // Blinking state array for each LED
static int blink_counters[MAX_DEVICES] = {0}; // Blink count array for each LED
static int blink_rates[MAX_DEVICES] = {0}; // Blink rate array for each LED

void led_blink_task(void *pvParameter)
{
    int led_id = *((int *)pvParameter);

    while (1)
    {
        // Wait for the semaphore
        if (xSemaphoreTake(xSemaphore[led_id], portMAX_DELAY) == pdTRUE)
        {
            // Toggle the LED based on the led_blinking state
            while (led_blinking[led_id])
            {
                gpio_set_level(LED_PIN + led_id, 1);
                vTaskDelay(blink_rates[led_id] / portTICK_PERIOD_MS); // LED on for 500ms

                gpio_set_level(LED_PIN + led_id, 0);
                vTaskDelay(blink_rates[led_id] / portTICK_PERIOD_MS); // LED off for 500ms

                // Check if the led_blinking state has changed or blink count reached
                if (xSemaphoreTake(xSemaphore[led_id], 0) == pdTRUE || (blink_counters[led_id] > 0 && --blink_counters[led_id] == 0))
                {
                    // set led to his state before blinking
                    gpio_set_level(LED_PIN + led_id, led_states[led_id]);
                    break;
                }
            }
        }
    }
}

bool init_led()
{
    esp_err_t ret;

    for (int i = 0; i < MAX_DEVICES; i++)
    {
        ESP_LOGI(LED_TAG, "Initializing LED %d pin ...", i);
        ret = gpio_set_direction(LED_PIN + i, GPIO_MODE_OUTPUT);
        if (ret != ESP_OK)
        {
            ESP_LOGE(LED_TAG, "Failed to set LED pin direction for LED %d: %s", i, esp_err_to_name(ret));
            return false;
        }

        ESP_LOGI(LED_TAG, "Creating semaphore for LED %d", i);
        xSemaphore[i] = xSemaphoreCreateBinary();

        if (xSemaphore[i] == NULL)
        {
            ESP_LOGE(LED_TAG, "Failed to create semaphore for LED %d", i);
            return false;
        }

        int *led_id = malloc(sizeof(int));
        *led_id = i;

        ESP_LOGI(LED_TAG, "Creating led_blink_task for LED %d", i);

        int result = xTaskCreate(&led_blink_task, "led_blink_task", 2048, led_id, 5, NULL);
        if (result != pdPASS)
        {
            ESP_LOGE(LED_TAG, "Failed to create led_blink_task for LED %d", i);
            return false;
        }

        ESP_LOGI(LED_TAG, "LED %d pin initialized", i);
    }
    return true;
}

void set_led(int led_id, bool state)
{
    if (led_id < 0 || led_id >= MAX_DEVICES)
    {
        ESP_LOGE(LED_TAG, "Invalid LED ID: %d", led_id);
        return;
    }
    gpio_set_level(LED_PIN + led_id, state);
    #if LOG_LED
        ESP_LOGI(LED_TAG, "LED %d set to %d", led_id, state);
    #endif
    led_states[led_id] = state;
}

bool get_led(int led_id)
{
    if (led_id < 0 || led_id >= MAX_DEVICES)
    {
        ESP_LOGE(LED_TAG, "Invalid LED ID: %d", led_id);
        return false;
    }
    return gpio_get_level(LED_PIN + led_id);
}

void start_led_blink(int led_id, int blink_count, int blink_rate)
{
    if (led_id < 0 || led_id >= MAX_DEVICES)
    {
        ESP_LOGE(LED_TAG, "Invalid LED ID: %d", led_id);
        return;
    }

    led_blinking[led_id]++;
    blink_counters[led_id] = blink_count;
    blink_rates[led_id] = blink_rate;
    #if LOG_LED
        ESP_LOGI(LED_TAG, "LED %d blinking started (%d)", led_id, led_blinking[led_id]);
    #endif
    xSemaphoreGiveFromISR(xSemaphore[led_id], NULL);
}

void stop_led_blink(int led_id)
{
    if (led_id < 0 || led_id >= MAX_DEVICES)
    {
        ESP_LOGE(LED_TAG, "Invalid LED ID: %d", led_id);
        return;
    }

    if (!led_blinking[led_id])
    {
        #if LOG_LED
            ESP_LOGI(LED_TAG, "LED %d blinking already stopped", led_id);
        #endif
        return;
    }

    led_blinking[led_id]--;
    blink_counters[led_id] = 0;
    set_led(led_id, led_states[led_id]);

    #if LOG_LED
        if (!led_blinking[led_id])
        {
            ESP_LOGI(LED_TAG, "LED %d blinking stopped (%d)", led_id, led_blinking[led_id]);
        }
        else
        {
            ESP_LOGI(LED_TAG, "LED %d blinking decreased (%d)", led_id, led_blinking[led_id]);
        }
    #endif
    xSemaphoreGiveFromISR(xSemaphore[led_id], NULL);
}