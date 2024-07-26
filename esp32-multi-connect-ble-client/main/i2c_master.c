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
    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    return ret;
}

bool i2c_master_write_slave(uint8_t *data_wr, size_t length)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    // Envoie des données à l'esclave
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_SLAVE_ADDR, &data_wr, length, 1000 / portTICK_PERIOD_MS);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Data sent: %s", data_wr);
    }
    else
    {
        ESP_LOGE(TAG, "Error sending data: %s", esp_err_to_name(ret));
    }
    return ret;
}

bool i2c_master_read_slave()
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    // Lecture des données de l'esclave
    ret = i2c_master_read_from_device(I2C_MASTER_NUM, I2C_SLAVE_ADDR, &data_received, 1, 1000 / portTICK_PERIOD_MS);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Data received: 0x%02X", data_received);
    }
    else
    {
        ESP_LOGE(TAG, "Error receiving data: %s", esp_err_to_name(ret));
    }
    return ret;
}