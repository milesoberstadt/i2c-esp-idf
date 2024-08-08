#include <Arduino.h>

// Configuration
#define SERIAL1_BAUDRATE 4800  // Set the baudrate according to your device specification
#define RESPONSE_TIME 50 // ms
byte speed_frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};  // SPEED
byte direction_frame[] = {0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x38};  // DIRECTION

void setup_wind() {

    // Start serial communication with the device on Serial1
    Serial1.begin(SERIAL1_BAUDRATE);

    // Give some time to initialize the serial communication
    delay(500);

    Serial.println("Connected to Serial1 at 4800 baud.");
}

float get_wind_speed() {
    // Send the speed inquiry frame
    Serial1.write(speed_frame, sizeof(speed_frame));

    // Wait for a moment to receive the response
    delay(RESPONSE_TIME);

    // Check if data is available
    if (Serial1.available() > 0) {
        byte response[7];  // Assuming the response is 7 bytes long
        int index = 0;

        // Read the response
        while (Serial1.available() > 0 && index < sizeof(response)) {
            response[index++] = Serial1.read();
        }

        // Process the response to extract wind speed
        if (index >= 5) {  // Ensure we have enough data
            int wind_speed = (response[3] << 8) | response[4];  // Combine bytes 4 and 5
            return wind_speed / 10.0;  // Convert to m/s
        } else {
            Serial.println("Invalid speed response.");
        }
    } else {
        Serial.println("No speed response received.");
    }
    return -1;  // Return -1 to indicate an error
}

int get_wind_direction() {
    // Send the direction inquiry frame
    Serial1.write(direction_frame, sizeof(direction_frame));

    // Wait for a moment to receive the response
    delay(RESPONSE_TIME);

    // Check if data is available
    if (Serial1.available() > 0) {
        byte response[9];
        int index = 0;

        // Read the response
        while (Serial1.available() > 0 && index < sizeof(response)) {
            response[index++] = Serial1.read();
        }

        // Process the response to extract wind direction
        if (index >= 5) {  // Ensure we have enough data
            return response[4] & 0x07;  // Extract bits 0-2 from byte 5
        } else {
            Serial.println("Invalid direction response.");
        }
    } else {
        Serial.println("No direction response received.");
    }
    return -1;  // Return -1 to indicate an error
}
