#include "display.h"

SSD1306_t dev;

void init_display()
{

    #if CONFIG_I2C_INTERFACE
    ESP_LOGI(TAG, "INTERFACE is i2c");
    ESP_LOGI(TAG, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
    ESP_LOGI(TAG, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
    ESP_LOGI(TAG, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    #endif // CONFIG_I2C_INTERFACE

    #if CONFIG_SPI_INTERFACE
    ESP_LOGI(TAG, "INTERFACE is SPI");
    ESP_LOGI(TAG, "CONFIG_MOSI_GPIO=%d",CONFIG_MOSI_GPIO);
    ESP_LOGI(TAG, "CONFIG_SCLK_GPIO=%d",CONFIG_SCLK_GPIO);
    ESP_LOGI(TAG, "CONFIG_CS_GPIO=%d",CONFIG_CS_GPIO);
    ESP_LOGI(TAG, "CONFIG_DC_GPIO=%d",CONFIG_DC_GPIO);
    ESP_LOGI(TAG, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO);
    #endif // CONFIG_SPI_INTERFACE

    #if CONFIG_FLIP
    dev._flip = true;
    ESP_LOGW(TAG, "Flip upside down");
    #endif

    #if CONFIG_SSD1306_128x64
    ESP_LOGI(TAG, "Panel is 128x64");
    ssd1306_init(&dev, 128, 64);
    #endif // CONFIG_SSD1306_128x64
    #if CONFIG_SSD1306_128x32
    ESP_LOGI(TAG, "Panel is 128x32");
    ssd1306_init(&dev, 128, 32);
    #endif // CONFIG_SSD1306_128x32

    ssd1306_clear_screen(&dev, false);
    ssd1306_contrast(&dev, 0xff);

}

void display_text(char *text, size_t len, size_t line)
{
    ssd1306_clear_screen(&dev, false);
    ssd1306_display_text(&dev, line, text, len, false);
    ssd1306_show_buffer(&dev);
}

void display_device(size_t idx, device_t device) {

    char* type = malloc(2+DEV_TYPE_STR_LEN*sizeof(char));
    type[0] = ' ';
    type[1] = '0'+idx;
    type[2] = ':';
    device_type_str(device.type, type+3);
    ssd1306_display_text(&dev, 0, type, strlen(type), false);

    char* state = malloc(DEV_STATE_STR_LEN+1*sizeof(char));
    state[0] = ' ';
    device_state_str(device.state, state+1);
    ssd1306_display_text(&dev, 1, state, strlen(state), false);

    char* battery = malloc(6*sizeof(char));
    sprintf(battery, " %d%%", device.battery_level);
    ssd1306_display_text(&dev, 2, battery, strlen(battery), false);

    if (device.value_size > 0) {
        char* value = (char*)device.value;
        ssd1306_display_text(&dev, 5, value, device.value_size, false);
    } else {
        ssd1306_clear_line(&dev, 5, false);
    }

    ssd1306_show_buffer(&dev);

    free(type);
    free(state);
    free(battery);

}