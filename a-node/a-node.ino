#include <ArduinoBLE.h>
#include "LSM6DS3.h"
#include "Wire.h"
#include <stdio.h>

#define SERVICE_UUID "0c7e1964-3616-4132-bbab-86afff1d9654"
#define WIND_UUID "e2238e3b-702c-406f-bd63-b3e977307e1e"

#define DISCOVERY_INTERVAL 1000  // Ms
#define SEND_INTERVAL 10         // Hz
#define BUTTON_PIN 0
#define sensorPin 1
int sensorValue = 0;
float sensorVoltage = 0;
float windSpeed = 0;
const float referenceVoltage = 5.0;  // (0-5V)
const float maxWindSpeed = 30.0;     // (0-30 m/s)

BLEService dataService(SERVICE_UUID);
BLECharacteristic windCharacteristic(WIND_UUID, BLERead | BLEWrite, 100);

// Create a instance of class LSM6DS3
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
  dataService.addCharacteristic(windCharacteristic);
  BLE.addService(dataService);

  Serial.println("XIAO BLE Sense (Peripheral)");
  Serial.println(" ");
}

void loop() {
  static unsigned long discoveryStartTime = 0;
  // TEST, TO REMOVE
  sensorValue = analogRead(sensorPin);
  sensorVoltage = sensorValue * (referenceVoltage / 1023.0);
  windSpeed = (sensorVoltage / referenceVoltage) * maxWindSpeed;
  char windSpeedData[100];
  sprintf(windSpeedData, "%.2f;%.2f;%.2f", (float)sensorValue, sensorVoltage, windSpeed);
  windCharacteristic.writeValue(windSpeedData);
  Serial.println(windSpeedData);
  delay(2000);
  // END
  // Check if the button is pressed
  if (digitalRead(BUTTON_PIN) == LOW) {
    discoveryStartTime = millis();
    BLE.advertise();
    Serial.println("- Starting Bluetooth discovery for 30 seconds...");
    delay(1000);  // Debounce delay
  }

  // If discovery has started, continue for 30 seconds
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

        // Read wind speed sensor
        sensorValue = analogRead(sensorPin);
        sensorVoltage = sensorValue * (referenceVoltage / 1023.0);
        windSpeed = (sensorVoltage / referenceVoltage) * maxWindSpeed;

        // Format and send wind speed data
        char windSpeedData[100];
        sprintf(windSpeedData, "%.2f;%.2f;%.2f", (float)sensorValue, sensorVoltage, windSpeed);
        windCharacteristic.writeValue(windSpeedData);
        Serial.print(windSpeedData);

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
