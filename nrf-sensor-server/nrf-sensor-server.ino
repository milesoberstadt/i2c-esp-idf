#include <ArduinoBLE.h>
#include "LSM6DS3.h"
#include "Wire.h"
#include <stdio.h>

/* UUIDS */

#define SERVICE_UUID "19b10000-e8f2-537e-4f6c-d104768a1214"
#define GYRO_UUID "31d31ed5-aa9b-4325-b011-25caa3765c2a"
#define ACCEL_UUID "bcd6dfbe-0c7b-4530-a5b3-ecd2ed69ff4f"

/* TIME & DURATIONS */

#define DISCOVERY_INTERVAL 1000 // Ms
#define DISCOVERY_DURATION 30 //seconds
#define SEND_INTERVAL 10 // Hz

/* BUTTON */

// if button is disabled, the nRF will always advertise when not connected
#define USE_BUTTON 1
#define BUTTON_PIN 0

/* LED */

#define LED_PIN LED_BUILTIN

BLEService dataService(SERVICE_UUID); 
BLECharacteristic gyroCharacteristic(GYRO_UUID, BLERead | BLENotify, 100);
BLECharacteristic accelCharacterictic(ACCEL_UUID, BLERead | BLENotify, 100);

bool isAdvertising = false;

// Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    // I2C device address 0x6A

void setup() {
  pinMode(LED_PIN, OUTPUT);
  #if USE_BUTTON
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Initialize the button pin
  #endif

  Serial.begin(9600);

  if (myIMU.begin() != 0) {
      Serial.println("IMU error");
  } else {
      Serial.println("IMU OK!");
  }
  
  if (!BLE.begin()) {
    Serial.println("- Starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  BLE.setLocalName("XIAO BLE Sense (Peripheral)");
  BLE.setAdvertisedService(dataService);
  dataService.addCharacteristic(gyroCharacteristic);
  dataService.addCharacteristic(accelCharacterictic);
  BLE.addService(dataService);
  BLE.poll();

  Serial.println("XIAO BLE Sense (Peripheral)");
  Serial.println(" ");
}

void loop() {
  #if USE_BUTTON
    static unsigned long discoveryStartTime = 0;
  #endif

  BLEDevice central = BLE.central();

  // when device is connected
  if (central) {
      Serial.println("* Connected to central device!");
      Serial.print("* Device MAC address: ");
      Serial.println(central.address());
      Serial.println(" ");

      while (central.connected()) {
        digitalWrite(LED_PIN, LOW);

        char gyro[100];
        sprintf(gyro, "%.2f;%.2f;%.2f", myIMU.readFloatGyroX(), myIMU.readFloatGyroY(), myIMU.readFloatGyroZ());
        gyroCharacteristic.writeValue(gyro);
        Serial.println(gyro);

        char accelerometer[100];
        sprintf(accelerometer, "%.2f;%.2f;%.2f", myIMU.readFloatAccelX(), myIMU.readFloatAccelY(), myIMU.readFloatAccelZ());
        accelCharacterictic.writeValue(accelerometer);
        Serial.println(accelerometer);

        delay(1000 / SEND_INTERVAL);
      }
      
      Serial.println("* Disconnected from central device!");
    } 

    bool shouldScan;
    #if USE_BUTTON 
      shouldScan = discoveryStartTime > 0 && millis() - discoveryStartTime < DISCOVERY_DURATION*1000;
    #else
      shouldScan = true;
    #endif

    // start scan if it should
    if (!isAdvertising && shouldScan) {
      BLE.advertise();
      isAdvertising = true;
      Serial.println("- Starting Bluetooth discovery for 30 seconds...");
      delay(1000); // Debounce delay
    }

    // stop scan if it should
    if (isAdvertising && !shouldScan) {
      BLE.stopAdvertise();
      isAdvertising = false;
      digitalWrite(LED_BUILTIN, 0);
      #if USE_BUTTON
        discoveryStartTime = 0; // Reset discovery start time
      #endif
    }

    #if USE_BUTTON
        // start scan on button press
      if (!isAdvertising && digitalRead(BUTTON_PIN) == LOW) {
        discoveryStartTime = millis();
      }
    #endif

    // blinking led during scan
    if (isAdvertising) {
      int status = digitalRead(LED_PIN);
      digitalWrite(LED_PIN, !status);
      delay(DISCOVERY_INTERVAL);
    }

}