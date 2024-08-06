#include <ArduinoBLE.h>
#include "LSM6DS3.h"
#include "Wire.h"
#include <stdio.h>

/* --- * Definitions * --- */

/* --- NODE TYPE --- */

#define M_NODE 1
#define A_NODE 2

/* Change this value to compile a node */
#define NODE_TYPE A_NODE

/* --- UUIDS --- */

// M-Node
#define M_NODE_SERVICE_UUID "19b10000-e8f2-537e-4f6c-d104768a1214"
#define GYRO_UUID "31d31ed5-aa9b-4325-b011-25caa3765c2a"
#define ACCEL_UUID "bcd6dfbe-0c7b-4530-a5b3-ecd2ed69ff4f"

// A-Node
#define A_NODE_SERVICE_UUID "0c7e1964-3616-4132-bbab-86afff1d9654"
#define WIND_SPEED_UUID "e2238e3b-702c-406f-bd63-b3e977307e1e"
#define WIND_DIRECTION_UUID "fbf9ad3a-cef4-41d9-a08b-06f8424a1fb0"

/* --- TIME & DURATIONS --- */

#define DISCOVERY_INTERVAL 1000 // Ms
#define DISCOVERY_DURATION 30 //seconds
#define SEND_INTERVAL 10 // Hz

/* --- BUTTON --- */

// if button is disabled, the nRF will always advertise when not connected
#define USE_BUTTON 1
#define BUTTON_PIN 0

/* --- PINS --- */

#define LED_PIN LED_BUILTIN

/* --- A-Node config --- */

#define ANEMOMETER_PIN 1 // for A-Node only
#define REFERENCE_VOLTAGE 5.0 // (V)
#define MAX_WIND_SPEED 30.0 // (m/s)


/* --- * Declarations * --- */

bool isAdvertising = false;

/* For M-Node */
#if NODE_TYPE == M_NODE
  BLEService dataService(M_NODE_SERVICE_UUID); 
  BLECharacteristic gyroCharacteristic(GYRO_UUID, BLERead | BLENotify, 100);
  BLECharacteristic accelCharacterictic(ACCEL_UUID, BLERead | BLENotify, 100);
  // Create a instance of class LSM6DS3
  LSM6DS3 myIMU(I2C_MODE, 0x6A);    // I2C device address 0x6A
#endif

/* For A-Node */
#if NODE_TYPE == A_NODE
  BLEService dataService(A_NODE_SERVICE_UUID);

  BLEUnsignedIntCharacteristic windSpeedCharacteristic(WIND_SPEED_UUID, BLERead | BLENotify);
  BLEUnsignedIntCharacteristic windDirectionCharacteristic(WIND_DIRECTION_UUID, BLERead | BLENotify); 

  int speedSensorValue = 0;
  float speedSensorVoltage = 0;
  float windSpeed = 0;

  bool windSpeedEnabled = false;
  bool windDirectionEnabled = false;

  const byte O2[] = {0x01 ,0x03 ,0x00 ,0x00 ,0x00 ,0x02 ,0xC4 ,0x0B};
  byte windDirectionValues[20];
  int windDirection = 0;

  int calculateWindDirection() {
    if (Serial1.write(O2, sizeof(O2)) == 8) {
      for (byte i = 0; i < 11; i++) {
        windDirectionValues[i] = Serial1.read();
      }
    }
    return ((windDirectionValues[5]*256)+windDirectionValues[6]);
  }

#endif


/* --- * Setup & Loop * --- */

void setup() {

  pinMode(LED_PIN, OUTPUT);
  #if USE_BUTTON
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Initialize the button pin
  #endif

  Serial.begin(9600);
  
  /* M_NODE SETUP */
  #if NODE_TYPE == M_NODE
    if (myIMU.begin() != 0) {
        Serial.println("IMU error");
    } else {
        Serial.println("IMU OK!");
    }
    dataService.addCharacteristic(gyroCharacteristic);
    dataService.addCharacteristic(accelCharacterictic);

    Serial.println("XIAO BLE Sense (M-Node)");
    BLE.setLocalName("XIAO BLE Sense (M-Node)");
  #endif

  /* A_NODE SETUP */
  #if NODE_TYPE == A_NODE

    // Init wind direction sensor
    Serial1.begin(4800);

    if (Serial1.available() > 0) {
      Serial.println("Wind direction sensor OK!");
      windDirectionEnabled = true;
      dataService.addCharacteristic(windDirectionCharacteristic);
    } else {
      Serial.println("Wind direction sensor error");
    }

    // Read initial value from anemometer sensor
    speedSensorValue = analogRead(ANEMOMETER_PIN);
    if (speedSensorValue == 0) {
      Serial.println("Anemometer error");
    } else {
      Serial.println("Anemometer OK!");    
      windSpeedEnabled = true;
      dataService.addCharacteristic(windSpeedCharacteristic);
    }

    Serial.println("XIAO BLE Sense (A-Node)");
    BLE.setLocalName("XIAO BLE Sense (A-Node)");

  #endif
  
  if (!BLE.begin()) {
    Serial.println("- Starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  BLE.setAdvertisedService(dataService);
  BLE.addService(dataService);

  setup_battery();

  BLE.poll();

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

        #if NODE_TYPE == M_NODE
          /* M_NODE SENDING DATA */

          char gyro[100];
          sprintf(gyro, "%.2f;%.2f;%.2f", myIMU.readFloatGyroX(), myIMU.readFloatGyroY(), myIMU.readFloatGyroZ());
          gyroCharacteristic.writeValue(gyro);
          Serial.println(gyro);

          char accelerometer[100];
          sprintf(accelerometer, "%.2f;%.2f;%.2f", myIMU.readFloatAccelX(), myIMU.readFloatAccelY(), myIMU.readFloatAccelZ());
          accelCharacterictic.writeValue(accelerometer);
          Serial.println(accelerometer);
        #endif


        #if NODE_TYPE == A_NODE
        /* A_NODE SENDING DATA */

        // wind speed
        if (windDirectionEnabled) {
          // Read
          speedSensorValue = analogRead(ANEMOMETER_PIN);
          speedSensorVoltage = speedSensorValue * (REFERENCE_VOLTAGE / 1023.0);
          windSpeed = (speedSensorVoltage / REFERENCE_VOLTAGE) * MAX_WIND_SPEED;

          // Send data
          windSpeedCharacteristic.writeValue(windSpeed);
          Serial.print("Wind speed: ");
          Serial.println(windSpeed);
        }

        // wind direction
        if (windDirectionEnabled) {
          windDirection = calculateWindDirection();
          windDirectionCharacteristic.writeValue(windDirection);
          Serial.print("Wind direction: ");
          Serial.println(windDirection);
        }
          
        #endif

        loop_battery();

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