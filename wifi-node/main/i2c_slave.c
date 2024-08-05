#include "i2c_slave.h"

void i2c_slave_init()
{
    i2c_config_t conf_slave = {
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .mode = I2C_MODE_SLAVE,
        .slave.addr_10bit_en = 0,
        .slave.slave_addr = I2C_SLAVE_ADDR,
    };
    i2c_param_config(I2C_SLAVE_NUM, &conf_slave);
    i2c_driver_install(I2C_SLAVE_NUM, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
    ESP_LOGI(i2c_TAG, "I2C slave initialized");
}

uint8_t i2c_slave_receive()
{
    uint8_t data_received;
    // Réception des données du maître
    int ret = i2c_slave_read_buffer(I2C_SLAVE_NUM, &data_received, 1, portMAX_DELAY);
    if (ret > 0)
    {
        ESP_LOGI(i2c_TAG, "Data received: 0x%02X", data_received);
    }
    else
    {
        ESP_LOGE(i2c_TAG, "Failed to receive data");
    }
    return data_received;
}

void i2c_slave_send(uint8_t data_to_send)
{
    // Envoie des données au maître
    int ret = i2c_slave_write_buffer(I2C_SLAVE_NUM, &data_to_send, 1, portMAX_DELAY);
    if (ret > 0)
    {
        ESP_LOGI(i2c_TAG, "Data sent: 0x%02X", data_to_send);
    }
    else
    {
        ESP_LOGE(i2c_TAG, "Failed to send data");
    }
}