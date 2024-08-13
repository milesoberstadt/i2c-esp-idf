#include "i2c_master.h"

i2c_master_dev_handle_t dev_handle;

bool i2c_master_init() {

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_WIFI_SLAVE_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    return true;

}

bool i2c_master_write_slave(uint8_t *data_wr, size_t len, uint8_t addr) {

    esp_err_t ret = i2c_master_transmit(dev_handle, data_wr, len, -1);

    if (ret != ESP_OK)
    {
        ESP_LOGE(I2C_TAG, "Error sending data: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}