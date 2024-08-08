#include "slave.h"

I2CSlave::I2CSlave() {
    Wire.begin(I2C_SLAVE_SDA, I2C_SLAVE_SCL, I2C_SLAVE_ADDR);
    Wire.setClock(100000); // Set I2C clock speed
    Wire.onReceive(I2CSlave::on_receive_static);
    Wire.onRequest(I2CSlave::on_request_static);
}

I2CSlave::~I2CSlave() {
    // Destructor
}

void I2CSlave::on_receive_static(int byteCount) {
    I2CSlave::instance().on_receive(byteCount);
}

void I2CSlave::on_request_static() {
    I2CSlave::instance().on_request();
}

void I2CSlave::on_receive(int byteCount) {
    if (byteCount < sizeof(message_t) + sizeof(device_t)) {
        // Not enough data to form a valid message
        return;
    }

    uint8_t buffer[byteCount];
    Wire.readBytes(buffer, byteCount);
    process_message(buffer, byteCount);
}

void I2CSlave::on_request() {
    // Placeholder for handling data request from master
    // You can implement your logic to send data back to the master if needed
}

void I2CSlave::process_message(uint8_t* data, size_t length) {

    message_t msg_type;
    uint8_t dev_idx;
    uint8_t msg_len;
    
    // Extract message header
    memcpy(&msg_type, data, sizeof(message_t));
    data += sizeof(message_t);

    memcpy(&dev_idx, data, sizeof(uint8_t));
    data += sizeof(uint8_t);

    memcpy(&msg_len, data, sizeof(uint8_t));
    data += sizeof(uint8_t);

    // Handle devices message
    switch (msg_type) {
        case msg_err:
            Serial.printf("Error received\n");
            break;
        case msg_init_start:
            Serial.printf("Initialisation started\n");
            break;
        case msg_init_end:
            Serial.printf("Initialisation end\n");
            break;
        case msg_dev_type:
            {
                device_type_t dev_type = (device_type_t) data[0];

                Serial.printf("Device %d type: %s\n", dev_idx, device_type_str(dev_type).c_str());

                Devices::instance().set_device_type(dev_idx, dev_type);
            }
            break;
        case msg_dev_state:
            {
                device_state_t dev_state = (device_state_t) data[0];

                Serial.printf("Device %d state: %s\n", dev_idx, device_state_str(dev_state).c_str());

                Devices::instance().set_device_state(dev_idx, dev_state);
            }
            break;
        case msg_dev_data:

            Serial.printf("Device %d data: %s\n", dev_idx, device_value_str(data, msg_len).c_str());

            Devices::instance().set_device_value(dev_idx, data, msg_len);

            break;
        case msg_dev_selected:

            Serial.printf("Device %d selected\n", dev_idx);

            break;
        case msg_dev_error:
            
            Serial.printf("Device %d error\n", dev_idx);

            break;
        case msg_screen_off:

            Serial.printf("Screen off\n");

            start_sleeping();

            break;
        default:
            Serial.println("Unknown message received\n");
            break;
    }
}
