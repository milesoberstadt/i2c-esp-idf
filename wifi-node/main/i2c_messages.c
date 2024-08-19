#include "i2c_messages.h"

#define HEADER_SIZE 3

void process_message(uint8_t* data, size_t length) {

    esp_log_buffer_hex(i2c_MESSAGES, data, length);

    uint8_t msg_type = data[0];
    uint8_t dev_idx = data[1];
    uint8_t msg_len = data[3];

    ESP_LOGI(i2c_MESSAGES, "Message type: %d\n", msg_type);

    // Handle devices message
    switch (msg_type) {
        case msg_err:
            ESP_LOGI(i2c_MESSAGES, "Error received\n");
            break;
        case msg_init_start:
            ESP_LOGI(i2c_MESSAGES, "Initialisation started\n");
            break;
        case msg_init_end:
            ESP_LOGI(i2c_MESSAGES, "Initialisation end\n");
            break;
        case msg_dev_type:
            {
                device_type_t dev_type = data[0];

                set_device_type(dev_idx, dev_type);

                #if LOG_I2C_MESSAGES == 1
                // char* type_str = malloc(DEV_TYPE_STR_LEN*sizeof(char));
                // device_type_str(dev_type, type_str);
                // ESP_LOGI(i2c_MESSAGES, "Device %d type: %s\n", dev_idx, type_str);
                // free(type_str);
                #endif

            }
            break;
        case msg_dev_state:
            {
                device_state_t dev_state = data[0];

                set_device_state(dev_idx, dev_state);

                #if LOG_I2C_MESSAGES == 1
                // char* state_str = malloc(DEV_STATE_STR_LEN*sizeof(char));
                // device_state_str(dev_state, state_str);
                // ESP_LOGI(i2c_MESSAGES, "Device %d state: %s\n", dev_idx, state_str);
                // free(state_str);
                #endif

            }
            break;
        case msg_dev_data:

            set_device_value(dev_idx, data+HEADER_SIZE, msg_len);

            break;
        case msg_dev_selected:

            select_device(dev_idx);

            #if LOG_I2C_MESSAGES == 1
            ESP_LOGI(i2c_MESSAGES, "Device %d selected\n", dev_idx);
            #endif

            break;
        case msg_dev_error:
            
            // #if LOG_I2C_MESSAGES == 1
            ESP_LOGI(i2c_MESSAGES, "Device %d error\n", dev_idx);
            // #endif

            break;
        case msg_screen_toggle:

            // #if LOG_I2C_MESSAGES == 1
            ESP_LOGI(i2c_MESSAGES, "Screen toggle\n");
            // #endif

            // start_sleeping();

            break;
        default:
            ESP_LOGI(i2c_MESSAGES,"Unknown message received\n");
            break;
    }
}
