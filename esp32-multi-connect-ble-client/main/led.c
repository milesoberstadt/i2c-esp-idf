#include "led.h"

#define LED_DELAY 500 // Blink delay in milliseconds

static SemaphoreHandle_t xSemaphore[PROFILE_NUM] = {NULL}; // Semaphore array for each LED
static int led_blinking[PROFILE_NUM] = {0}; // Blinking state array for each LED
static int blink_counters[PROFILE_NUM] = {0}; // Blink count array for each LED

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
                set_led(led_id, 1);
                vTaskDelay(LED_DELAY / portTICK_PERIOD_MS); // LED on for 500ms

                set_led(led_id, 0);
                vTaskDelay(LED_DELAY / portTICK_PERIOD_MS); // LED off for 500ms

                // Check if the led_blinking state has changed or blink count reached
                if (xSemaphoreTake(xSemaphore[led_id], 0) == pdTRUE || (blink_counters[led_id] > 0 && --blink_counters[led_id] == 0))
                {
                    break;
                }
            }
        }
    }
}

void init_led()
{
    esp_err_t ret;

    for (int i = 0; i < PROFILE_NUM; i++)
    {
        ret = gpio_set_direction(LED_PIN + i, GPIO_MODE_OUTPUT);
        if (ret != ESP_OK)
        {
            ESP_LOGE(LED_TAG, "Failed to set LED pin direction for LED %d: %s", i, esp_err_to_name(ret));
            return;
        }

        xSemaphore[i] = xSemaphoreCreateBinary();

        if (xSemaphore[i] == NULL)
        {
            ESP_LOGE(LED_TAG, "Failed to create semaphore for LED %d", i);
            return;
        }

        int *led_id = malloc(sizeof(int));
        *led_id = i;

        int result = xTaskCreate(&led_blink_task, "led_blink_task", 2048, led_id, 5, NULL);
        if (result != pdPASS)
        {
            ESP_LOGE(LED_TAG, "Failed to create led_blink_task for LED %d", i);
            return;
        }

        ESP_LOGI(LED_TAG, "LED %d pin initialized", i);
    }
}

void set_led(int led_id, bool state)
{
    gpio_set_level(LED_PIN + led_id, state);
}

bool get_led(int led_id)
{
    return gpio_get_level(LED_PIN + led_id);
}

void start_led_blink(int led_id, int blink_count)
{
    if (led_id < 0 || led_id >= PROFILE_NUM)
    {
        ESP_LOGE(LED_TAG, "Invalid LED ID: %d", led_id);
        return;
    }

    led_blinking[led_id]++;
    blink_counters[led_id] = blink_count;
    #if LOG_LED
        ESP_LOGI(LED_TAG, "LED %d blinking started (%d)", led_id, led_blinking[led_id]);
    #endif
    xSemaphoreGiveFromISR(xSemaphore[led_id], NULL);
}

void stop_led_blink(int led_id)
{
    if (led_id < 0 || led_id >= PROFILE_NUM)
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