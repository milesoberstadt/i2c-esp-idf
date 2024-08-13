#include "i2c_slave.h"

i2c_slave_dev_handle_t slave_handle;

uint8_t *data_rd;
uint32_t size_rd = 0;

QueueHandle_t s_receive_queue;

static IRAM_ATTR bool i2c_slave_rx_done_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data) {
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);

    return high_task_wakeup == pdTRUE;
}

void process_message_task(void *param) {
    i2c_slave_rx_done_event_data_t rx_data;
    while (1) {
        if (xQueueReceive(s_receive_queue, &rx_data, portMAX_DELAY)) {
            process_message(data_rd, I2C_MESSAGE_MAX_LEN);
        }
    }
}

void init_i2c_slave() {
    i2c_slave_config_t i2c_slv_config = {
        .addr_bit_len = I2C_ADDR_BIT_LEN_7,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT_NUM,
        .send_buf_depth = 256,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .slave_addr = I2C_SLAVE_ADDR,
    };

    data_rd = (uint8_t *) malloc(I2C_MESSAGE_MAX_LEN * sizeof(uint8_t));

    ESP_ERROR_CHECK(i2c_new_slave_device(&i2c_slv_config, &slave_handle));
    
    s_receive_queue = xQueueCreate(1, sizeof(i2c_slave_rx_done_event_data_t));
    i2c_slave_event_callbacks_t cbs = {
        .on_recv_done = i2c_slave_rx_done_callback,
    };
    ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(slave_handle, &cbs, s_receive_queue));

    // xTaskCreatePinnedToCore(process_message_task, "process_message_task", 4096, NULL, 1, NULL, 0);

    ESP_LOGI(i2c_SLAVE, "I2C slave initialized");

    return;
}

void i2c_receive() {
    i2c_slave_rx_done_event_data_t rx_data;
    ESP_ERROR_CHECK(i2c_slave_receive(slave_handle, data_rd, I2C_MESSAGE_MAX_LEN));
    xQueueReceive(s_receive_queue, &rx_data, pdMS_TO_TICKS(10000));
}