#include "slave.h"

I2CSlave::I2CSlave() {
    // Constructor
}

I2CSlave::~I2CSlave() {
    // Destructor
}

void I2CSlave::begin() {
    Wire.begin(I2C_SLAVE_SDA, I2C_SLAVE_SCL, I2C_SLAVE_ADDR);
    Wire.setClock(100000); // Set I2C clock speed
    Wire.onReceive(I2CSlave::onReceiveStatic);
    Wire.onRequest(I2CSlave::onRequestStatic);
}

void I2CSlave::onReceiveStatic(int byteCount) {
    I2CSlave::getInstance().onReceive(byteCount);
}

void I2CSlave::onRequestStatic() {
    I2CSlave::getInstance().onRequest();
}

void I2CSlave::onReceive(int byteCount) {
    if (byteCount < sizeof(message_t) + sizeof(device_t)) {
        // Not enough data to form a valid message
        return;
    }

    uint8_t buffer[byteCount];
    Wire.readBytes(buffer, byteCount);
    processMessage(buffer, byteCount);
}

void I2CSlave::onRequest() {
    // Placeholder for handling data request from master
    // You can implement your logic to send data back to the master if needed
}

void I2CSlave::processMessage(uint8_t* data, size_t length) {
    message_t msgType;
    device_t device;
    // Extract message type
    memcpy(&msgType, data, sizeof(message_t));
    data += sizeof(message_t);

    // Extract device information
    memcpy(&device, data, sizeof(device_t));

    // Handle the message
    switch (msgType) {
        case pairing_message:
            Serial.printf("Device %d pairing\n", device.type);
            break;
        case connected_message:
            Serial.printf("Device %d connected\n", device.type);
            break;
        case disconnected_message:
            Serial.printf("Device %d disconnected\n", device.type);
            break;
        case disconnecting_message:
            Serial.printf("Device %d disconnecting\n", device.type);
            break;
        case connecting_message:
            Serial.printf("Device %d connecting\n", device.type);
            break;
        case data_message:
            Serial.printf("Device %d data received: ", device.type);
            for (int i = 0; i < device.value_size; i++) {
                Serial.printf("0x%02X ", device.value[i]);
            }
            Serial.println();
            break;
        default:
            Serial.println("Unknown message received");
            break;
    }
}
