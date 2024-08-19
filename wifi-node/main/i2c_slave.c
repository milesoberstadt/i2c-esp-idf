#include "i2c_slave.h"

i2c_slave_dev_handle_t slave_handle;

QueueHandle_t s_receive_queue;

uint8_t *data_rd;
uint32_t size_rd = 0;

static IRAM_ATTR bool i2c_slave_rx_done_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

void i2c_slave_init() {

    data_rd = (uint8_t *) malloc(I2C_DATA_LEN);

    i2c_slave_config_t i2c_slv_config = {
        .addr_bit_len = I2C_ADDR_BIT_LEN_7,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT_NUM,
        .send_buf_depth = 256,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .slave_addr = I2C_SLAVE_ADDR,
    };

    ESP_ERROR_CHECK(i2c_new_slave_device(&i2c_slv_config, &slave_handle));

    s_receive_queue = xQueueCreate(1, sizeof(i2c_slave_rx_done_event_data_t));
    i2c_slave_event_callbacks_t cbs = {
        .on_recv_done = i2c_slave_rx_done_callback,
    };
    ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(slave_handle, &cbs, s_receive_queue));

}

void i2c_receive_task(void *pvParameters) {
    uint8_t data_received[I2C_DATA_LEN];
    i2c_slave_rx_done_event_data_t rx_data;

    while (1) {
        esp_err_t ret = i2c_slave_receive(slave_handle, data_rd, I2C_DATA_LEN);

        if (ret != ESP_OK) {
            ESP_LOGE(i2c_SLAVE, "Error receiving data");
            vTaskDelay( pdMS_TO_TICKS( 1000 ));
            continue;
        }
        
        xQueueReceive(s_receive_queue, &rx_data, portMAX_DELAY);
        // Receive done.
        memcpy(data_received, rx_data.buffer, I2C_DATA_LEN);

        // Execute the action based on the received value.
        process_message(data_received, I2C_DATA_LEN);
    }
}

void i2c_start_receive() {
    xTaskCreate(i2c_receive_task, "i2c_receive_task", 4096, NULL, 5, NULL);
}