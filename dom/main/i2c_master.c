#include "i2c_master.h"

i2c_master_dev_handle_t dev_handle;
i2c_master_bus_handle_t bus_handle;

bool i2c_init() {
    ESP_LOGI(I2C_TAG, "Initializing I2C master on port %d, SDA: %d, SCL: %d", 
             I2C_PORT_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    // Try with I2C_NUM_0 instead of I2C_NUM_1
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,  // Changed from I2C_PORT_NUM to I2C_NUM_0
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = 1,
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "I2C master bus creation failed: %s", esp_err_to_name(ret));
        return false;
    }
    ESP_LOGI(I2C_TAG, "I2C master bus created successfully");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_SUB_SLAVE_ADDR,
        .scl_speed_hz = 100000,
    };

    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "I2C master device addition failed: %s", esp_err_to_name(ret));
        return false;
    }
    ESP_LOGI(I2C_TAG, "I2C master device added successfully");

    ESP_LOGI(I2C_TAG, "I2C master initialized successfully");
    return true;
}

bool i2c_write(uint8_t *data_wr) {
    esp_err_t ret = i2c_master_transmit(dev_handle, data_wr, I2C_DATA_LEN, 100);

    if (ret != ESP_OK) {
        ESP_LOGE(I2C_TAG, "Error sending data: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(I2C_TAG, "Data sent successfully");
    return true;
}