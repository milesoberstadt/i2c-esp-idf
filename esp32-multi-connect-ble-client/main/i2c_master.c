#include "i2c_master.h"

bool i2c_master_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    esp_err_t ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    return ret == ESP_OK;
}

bool i2c_master_write_slave(uint8_t *data_wr, size_t len, uint8_t addr)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    esp_err_t ret = i2c_master_write_to_device( I2C_MASTER_NUM, 
                                                addr, 
                                                data_wr, 
                                                len, 
                                                1000 / portTICK_PERIOD_MS);

    if (ret == ESP_OK)
    {
        ESP_LOGI(I2C_TAG, "Data sent: %s", data_wr);
    }
    else
    {
        ESP_LOGE(I2C_TAG, "Error sending data: %s", esp_err_to_name(ret));
    }
    return ret;
}

bool i2c_master_read_slave(uint8_t addr, uint8_t *data_received)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM, 
                                                addr, 
                                                data_received, 
                                                I2C_BUFFER_SIZE, 
                                                1000 / portTICK_PERIOD_MS);
    if (ret != ESP_OK)
    {
        ESP_LOGE(I2C_TAG, "Error receiving data: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}