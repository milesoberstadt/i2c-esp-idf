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

bool i2c_master_write_slave(uint8_t *data_wr)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Envoie des données à l'esclave
    esp_err_t ret = i2c_master_write_to_device( I2C_MASTER_NUM, 
                                                I2C_SLAVE_ADDR, 
                                                data_wr, 
                                                I2C_BUFFER_SIZE, 
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

bool i2c_master_read_slave()
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    // Lecture des données de l'esclave

    uint8_t data_received[I2C_BUFFER_SIZE];
    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM, 
                                                I2C_SLAVE_ADDR, 
                                                data_received, 
                                                I2C_BUFFER_SIZE, 
                                                1000 / portTICK_PERIOD_MS);
    if (ret == ESP_OK)
    {
        ESP_LOGI(I2C_TAG, "Data received: %s", data_received);
    }
    else
    {
        ESP_LOGE(I2C_TAG, "Error receiving data: %s", esp_err_to_name(ret));
    }
    return ret;
}