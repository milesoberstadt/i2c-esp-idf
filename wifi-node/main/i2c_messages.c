#include "i2c_messages.h"

#define HEADER_SIZE 2

void process_message(uint8_t* data, size_t length) {

    message_t msg_type = (message_t) data[0];
    uint8_t dev_idx = data[1];
    
    esp_log_buffer_hex(i2c_MESSAGES, data, length);

    display_text((char*)data, length);

    // Extract message header
    memcpy(&msg_type, data, sizeof(message_t));
    data += sizeof(message_t);

    memcpy(&dev_idx, data, sizeof(uint8_t));
    data += sizeof(uint8_t);

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
                device_type_t dev_type = (device_type_t) data[0];

                char* type_str = malloc(20*sizeof(char));
                device_type_str(dev_type, type_str, 20);

                ESP_LOGI(i2c_MESSAGES, "Device %d type: %s\n", dev_idx, type_str);

                free(type_str);

                // Devices::instance().set_device_type(dev_idx, dev_type);
            }
            break;
        case msg_dev_state:
            {
                device_state_t dev_state = (device_state_t) data[0];

                char* state_str = malloc(20*sizeof(char));
                device_state_str(dev_state, state_str, 20);

                ESP_LOGI(i2c_MESSAGES, "Device %d state: %s\n", dev_idx, state_str);

                free(state_str);

                // Devices::instance().set_device_state(dev_idx, dev_state);
            }
            break;
        case msg_dev_data:

            char* dev_data_str = malloc(I2C_DATA_LEN-HEADER_SIZE*sizeof(char));
            device_value_str(   data+HEADER_SIZE, 
                                dev_data_str,
                                I2C_DATA_LEN-HEADER_SIZE);

            ESP_LOGI(i2c_MESSAGES, "Device %d data: %s\n", dev_idx, dev_data_str);

            // Devices::instance().set_device_value(dev_idx, data, msg_len);

            free(dev_data_str);

            break;
        case msg_dev_selected:

            ESP_LOGI(i2c_MESSAGES, "Device %d selected\n", dev_idx);

            break;
        case msg_dev_error:
            
            ESP_LOGI(i2c_MESSAGES, "Device %d error\n", dev_idx);

            break;
        case msg_screen_toggle:

            ESP_LOGI(i2c_MESSAGES, "Screen toggle\n");

            // start_sleeping();

            break;
        default:
            ESP_LOGI(i2c_MESSAGES,"Unknown message received\n");
            break;
    }
}
