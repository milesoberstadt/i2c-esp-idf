/* --- NODE TYPE --- */

#define M_NODE 1
#define A_NODE 2

/* Change this value to compile a node */
#define NODE_TYPE A_NODE


/* --- * Libraries * --- */
#include <ArduinoBLE.h>
#if NODE_TYPE == M_NODE
  #include <LSM6DS3.h>
#endif


/* --- * Definitions * --- */

#define BAUDRATE 9600

// M-Node UUID
#define M_NODE_SERVICE_UUID "19b10000-e8f2-537e-4f6c-d104768a1214"
#define GYRO_UUID "31d31ed5-aa9b-4325-b011-25caa3765c2a"
#define ACCEL_UUID "bcd6dfbe-0c7b-4530-a5b3-ecd2ed69ff4f"

// A-Node UUID
#define A_NODE_SERVICE_UUID "0c7e1964-3616-4132-bbab-86afff1d9654"
#define WIND_SPEED_UUID "e2238e3b-702c-406f-bd63-b3e977307e1e"
#define WIND_DIRECTION_UUID "fbf9ad3a-cef4-41d9-a08b-06f8424a1fb0"

// Time & durations

#define DISCOVERY_INTERVAL 1000 // Ms
#define DISCOVERY_DURATION 30 //seconds
#define SEND_INTERVAL 10 // Hz

// Button
#define USE_BUTTON 1
#define BUTTON_PIN 9
// if button is disabled, the nRF will always advertise when not connected

// LED
#define LED_PIN LED_BUILTIN


/* --- * Declarations * --- */

int isAdvertising = 0;

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
  BLEFloatCharacteristic windSpeedCharacteristic(WIND_SPEED_UUID, BLERead | BLENotify);
  BLEUnsignedIntCharacteristic windDirectionCharacteristic(WIND_DIRECTION_UUID, BLERead | BLENotify); 
#endif

/* --- * Setup & Loop * --- */

void setup() {

  pinMode(LED_PIN, OUTPUT);
  #if USE_BUTTON
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Initialize the button pin
  #endif

  Serial.begin(BAUDRATE);
  
  /* M_NODE SETUP */
  #if NODE_TYPE == M_NODE
    Serial.println("Initialising M-Node ...");

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

    Serial.println("Initialising A-Node ...");

    setup_wind();

    dataService.addCharacteristic(windDirectionCharacteristic);
    dataService.addCharacteristic(windSpeedCharacteristic);

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

  BLEDevice central = BLE.central();

  // when device is connected
  if (central) {
      Serial.println("* Connected to central device!");
      Serial.print("* Device MAC address: ");
      Serial.println(central.address());
      Serial.println(" ");

      while (central.connected()) {
        digitalWrite(LED_PIN, HIGH);

        #if NODE_TYPE == M_NODE
          mnode_loop();
        #endif

        #if NODE_TYPE == A_NODE
          anode_loop();
        #endif

        loop_battery();

        delay(1000 / SEND_INTERVAL);
      }
      
      digitalWrite(LED_PIN, LOW);
      Serial.println("* Disconnected from central device!");
    } 

    scan_loop();

}

void scan_loop() {
  #if USE_BUTTON
    static unsigned long discoveryStartTime = 0;
  #endif

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
    digitalWrite(LED_PIN, LOW);
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

#if NODE_TYPE == A_NODE
void anode_loop() {

  float wind_speed = get_wind_speed();
  if (wind_speed != -1) {
      windSpeedCharacteristic.writeValue(wind_speed);
      Serial.print("Wind speed: ");
      Serial.print(wind_speed);
      Serial.println(" m/s");
  }

  int wind_direction = get_wind_direction();
  if (wind_direction != -1) {
      windDirectionCharacteristic.writeValue(wind_direction);
      Serial.print("Wind direction: ");
      Serial.println(wind_direction);
  }
}
#endif

#if NODE_TYPE == M_NODE
void mnode_loop() {

  char gyro[50];
  sprintf(gyro, "%.2f;%.2f;%.2f", myIMU.readFloatGyroX(), myIMU.readFloatGyroY(), myIMU.readFloatGyroZ());
  gyroCharacteristic.writeValue(gyro);
  Serial.println(gyro);

  char accelerometer[50];
  sprintf(accelerometer, "%.2f;%.2f;%.2f", myIMU.readFloatAccelX(), myIMU.readFloatAccelY(), myIMU.readFloatAccelZ());
  accelCharacterictic.writeValue(accelerometer);
  Serial.println(accelerometer);
}
#endif