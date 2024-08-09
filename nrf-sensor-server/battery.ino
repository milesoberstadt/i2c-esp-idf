#define BATTERY_SERVICE_UUID "bbd16cf5-9234-43eb-93b5-679e4142f689"
#define BATTERY_CHARGE_UUID "12fb1e82-a141-429a-86c1-6be3f0c561af"

#define BATTERY_SEND_INTERVAL 10000 // ms

#define BATTERY_PIN_MAX_VOLTAGE 5.5 // v -> the value to get 1023 on the pin
#define BATTERY_MAX_VOLTAGE 5 // v -> the voltage of the battery when fully charged
#define BATTERY_MIN_VOLTAGE 3.0 // v -> the voltage of the battery when it is considered empty

#define BATTERY_PIN 3 // ADC2

BLEService batteryService(BATTERY_SERVICE_UUID);
BLEUnsignedIntCharacteristic batteryChargeCharacteristic(BATTERY_CHARGE_UUID, BLERead | BLENotify);

unsigned long batterySendPreviousMillis = 0;

void setup_battery() {
    pinMode(BATTERY_PIN, INPUT);
    batteryService.addCharacteristic(batteryChargeCharacteristic);
    BLE.addService(batteryService);
}

int getBatteryCharge() {
    int batteryCharge = analogRead(BATTERY_PIN);
    float batteryVoltage = (batteryCharge * BATTERY_PIN_MAX_VOLTAGE) / 1023;
    int batteryPercentage = ((batteryVoltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100;
    if (batteryPercentage > 100) {
        return 100;
    }
    if (batteryPercentage < 0) {
        return 0;
    }
    return batteryPercentage;
}

void loop_battery() {

    unsigned long currentMillis = millis();

    if (currentMillis - batterySendPreviousMillis >= BATTERY_SEND_INTERVAL) {
        batterySendPreviousMillis = currentMillis;

        int batteryCharge = getBatteryCharge();
        Serial.print("Battery charge: ");
        Serial.println(batteryCharge);
        batteryChargeCharacteristic.writeValue(batteryCharge);
    }

}