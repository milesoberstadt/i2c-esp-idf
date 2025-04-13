/* --- NODE TYPE --- */

#define M_NODE 1
#define A_NODE 2
#define S_NODE 3

/* Change this value to compile a node */
#define NODE_TYPE M_NODE


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

// S-Node
#define S_NODE_SERVICE_UUID "63e4eb54-b0bc-4374-8d2a-5f08f951230a"
#define SLEEP_UUID "2c41ce1f-acd3-4088-8394-b21a88e88142"


/* --- TIME & DURATIONS --- */

#define DISCOVERY_DURATION  30    //seconds
#define SEND_INTERVAL       10    // Hz

// Button
#define USE_BUTTON  1
#define BUTTON_PIN  9
// if button is disabled, the nRF will always advertise when not connected

// Sleep pin
#define SLEEP_PIN   D0

/* --- * Declarations * --- */

int isAdvertising = 0;
int shouldScan = 0;

/* For M-Node */
#if NODE_TYPE == M_NODE
#define MNODE_BUFFER_SIZE 30
BLEService dataService(M_NODE_SERVICE_UUID); 
BLECharacteristic gyroCharacteristic(GYRO_UUID, BLERead | BLENotify, MNODE_BUFFER_SIZE);
BLECharacteristic accelCharacterictic(ACCEL_UUID, BLERead | BLENotify, MNODE_BUFFER_SIZE);
// Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    // I2C device address 0x6A
#endif

/* For A-Node */
#if NODE_TYPE == A_NODE
BLEService dataService(A_NODE_SERVICE_UUID);
BLEFloatCharacteristic windSpeedCharacteristic(WIND_SPEED_UUID, BLERead | BLENotify);
BLEUnsignedIntCharacteristic windDirectionCharacteristic(WIND_DIRECTION_UUID, BLERead | BLENotify);
#endif

#if NODE_TYPE == S_NODE
BLEService dataService(S_NODE_SERVICE_UUID);
BLEBooleanCharacteristic sleepCharacteristic(SLEEP_UUID, BLEWrite | BLENotify, 100);
#endif


void setup() {

  // Button setup
  #if USE_BUTTON
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Initialize the button pin
  #endif

  // Led setup
  led_setup();

  // Serial setup
  Serial.begin(BAUDRATE);

  /* S_NODE SETUP */
  #if NODE_TYPE == S_NODE
    snode_setup();
  #endif

  /* M_NODE SETUP */
  #if NODE_TYPE == M_NODE
    mnode_setup();
  #endif

  /* A_NODE SETUP */
  #if NODE_TYPE == A_NODE
    anode_setup();
  #endif

  /* BLE SETUP */
  if (!BLE.begin()) {
    Serial.println("- Starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }
  BLE.setAdvertisedService(dataService);
  BLE.addService(dataService);
  BLE.poll();

  setup_battery();

  Serial.println(" ");

  start_led_blink(0, 255, 0, 0, 200, 5000);

}

void loop() {

  BLEDevice central = BLE.central();

  // when device is connected
  if (central) {
    Serial.println("* Connected to central device!");
    Serial.print("* Device MAC address: ");
    Serial.println(central.address());
    Serial.println(" ");

    start_led_blink(0, 0, 255, 0, 200, 5000);

    while (central.connected()) {

      #if NODE_TYPE == M_NODE
      mnode_loop();
      #endif

      #if NODE_TYPE == A_NODE
      anode_loop();
      #endif

      #if NODE_TYPE == S_NODE
      snode_loop();
      #endif

      loop_battery();
      led_loop();

      delay(1000 / SEND_INTERVAL);
    }

    stop_led_blink(0);
    start_led_blink(0, 255, 0, 0, 200, 5000);
    Serial.println("* Disconnected from central device!");
  }

  scan_loop();
}

void scan_loop() {

  led_loop();

  #if USE_BUTTON
  static unsigned long discoveryStartTime = 0;
  #endif

  #if USE_BUTTON
  shouldScan = discoveryStartTime > 0 && millis() - discoveryStartTime < DISCOVERY_DURATION * 1000;
  #else
  shouldScan = 1;
  #endif

  // start scan if it should
  if (!isAdvertising && shouldScan) {
    BLE.advertise();
    isAdvertising = 1;
    Serial.println("- Starting Bluetooth discovery for 30 seconds...");
    start_led_blink(0, 0, 255, 0, 200, 200);
    delay(1000);  // Debounce delay
  }

  // stop scan if it should
  if (isAdvertising && !shouldScan) {
    BLE.stopAdvertise();
    isAdvertising = 0;
    Serial.println("Discovery ended.");
    stop_led_blink(0);
    #if USE_BUTTON
    discoveryStartTime = 0;  // Reset discovery start time
    #endif
  }

  #if USE_BUTTON
  // button press start scan timer
  if (!isAdvertising && digitalRead(BUTTON_PIN) == LOW) {
    discoveryStartTime = millis();
  }
  #endif

}

#if NODE_TYPE == A_NODE
void anode_setup() {
  Serial.println("Initialising A-Node ...");

  setup_wind();

  dataService.addCharacteristic(windSpeedCharacteristic);
  dataService.addCharacteristic(windDirectionCharacteristic);

  Serial.println("XIAO BLE Sense (A-Node)");
  BLE.setLocalName("XIAO BLE Sense (A-Node)");
}

void anode_loop() {

  pinMode(SLEEP_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SLEEP_PIN), handleSignalChange, CHANGE);

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
void mnode_setup() {
  Serial.println("Initialising M-Node ...");

  pinMode(SLEEP_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SLEEP_PIN), handleSignalChange, CHANGE);

  if (myIMU.begin() != 0) {
    Serial.println("IMU error");
  } else {
    Serial.println("IMU OK!");
  }

  dataService.addCharacteristic(gyroCharacteristic);
  dataService.addCharacteristic(accelCharacterictic);

  Serial.println("XIAO BLE Sense (M-Node)");
  BLE.setLocalName("XIAO BLE Sense (M-Node)");
}

void mnode_loop() {

  char gyro[MNODE_BUFFER_SIZE] = {0};
  sprintf(gyro, "%.2f;%.2f;%.2f\0", myIMU.readFloatGyroX(), myIMU.readFloatGyroY(), myIMU.readFloatGyroZ());
  gyroCharacteristic.writeValue(gyro);
  // Serial.println(gyro);

  char accelerometer[MNODE_BUFFER_SIZE] = {0};
  sprintf(accelerometer, "%.2f;%.2f;%.2f\0", myIMU.readFloatAccelX(), myIMU.readFloatAccelY(), myIMU.readFloatAccelZ());
  accelCharacterictic.writeValue(accelerometer);
  // Serial.println(accelerometer);
}
#endif

#if NODE_TYPE == S_NODE
void snode_setup() {
  NRF_POWER->DCDCEN = 1;
  pinMode(SLEEP_PIN, OUTPUT);
  dataService.addCharacteristic(sleepCharacteristic);
  digitalWrite(SLEEP_PIN, HIGH);  // By default send wake-up signal
}

void snode_loop() {
  if (sleepCharacteristic.valueUpdated()) {
    bool sleep_value = sleepCharacteristic.value();

    if (sleep_value) {
      // Sleep
      digitalWrite(SLEEP_PIN, LOW);
    } else {
      // Wake-up
      digitalWrite(SLEEP_PIN, HIGH);
    }
  }
}
#endif

// Fonction appelée sur interruption
void handleSignalChange() {
  if (digitalRead(SLEEP_PIN) == HIGH) {
    wakeUp();
  } else {
    enterSleepMode();
  }
}

// Fonction de réveil
void wakeUp() {
  bool shouldScan = true;
}

// Fonction de mise en veille
void enterSleepMode() {
  BLE.disconnect();
  NRF_POWER->SYSTEMOFF = 1;
}