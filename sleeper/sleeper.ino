#include <ArduinoBLE.h>
#include "LSM6DS3.h"
#include "Wire.h"
#include <stdio.h>

#define SERVICE_UUID "361ef722-706a-4df5-9ff2-cbc1478480c0"
#define BATTERY_UUID "f02f6ccd-ea68-4515-baf0-b8307e1ecb79"

#define DISCOVERY_INTERVAL 1000  // Ms
#define SEND_INTERVAL 10         // Hz
#define BUTTON_PIN 0
#define BATTERY_PIN 2

const float referenceVoltage = 3.3; // Reference voltage of ADC
const float voltageDividerRatio = 2.0; // Voltage divider ratio (e.g., if using 10k and 10k resistors, ratio is 2)

BLEService dataService(SERVICE_UUID);
BLECharacteristic batteryCharacteristic(BATTERY_UUID, BLERead | BLEWrite, 100);

LSM6DS3 myIMU(I2C_MODE, 0x6A);  // I2C device address 0x6A

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Initialize the button pin

  Serial.begin(9600);

  if (myIMU.begin() != 0) {
    Serial.println("IMU error");
  } else {
    Serial.println("IMU OK!");
  }

  if (!BLE.begin()) {
    Serial.println("- Starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }

  BLE.setLocalName("XIAO BLE Sense (Peripheral)");
  BLE.setAdvertisedService(dataService);
  dataService.addCharacteristic(batteryCharacteristic);
  BLE.addService(dataService);

  Serial.println("XIAO BLE Sense (Peripheral)");
  Serial.println(" ");
}

void loop() {
  static unsigned long discoveryStartTime = 0;

  if (digitalRead(BUTTON_PIN) == LOW) {
    discoveryStartTime = millis();
    BLE.advertise();
    Serial.println("- Starting Bluetooth discovery for 30 seconds...");
    delay(1000);  // Debounce delay
  }

  if (discoveryStartTime > 0 && millis() - discoveryStartTime < 30000) {
    BLE.poll();
    BLEDevice central = BLE.central();

    if (central) {
      Serial.println("* Connected to central device!");
      Serial.print("* Device MAC address: ");
      Serial.println(central.address());
      Serial.println(" ");

      while (central.connected()) {
        digitalWrite(LED_BUILTIN, LOW);

        // Measure battery voltage
        int sensorValue = analogRead(BATTERY_PIN);
        float batteryVoltage = (sensorValue * (referenceVoltage / 1023.0)) * voltageDividerRatio;

        // Format and send battery voltage
        char batteryData[100];
        sprintf(batteryData, "%.2f", batteryVoltage);
        batteryCharacteristic.writeValue(batteryData);
        Serial.print("Battery Voltage : ");
        Serial.print(batteryVoltage);
        Serial.println(" V");

        delay(1000 / SEND_INTERVAL);
      }

      Serial.println("* Disconnected from central device!");
    } else {
      int status = digitalRead(LED_BUILTIN);
      digitalWrite(LED_BUILTIN, !status);
    }
  } else {
    BLE.stopAdvertise();     // Stop advertising after 30 seconds
    discoveryStartTime = 0;  // Reset discovery start time
  }

  delay(500);
}