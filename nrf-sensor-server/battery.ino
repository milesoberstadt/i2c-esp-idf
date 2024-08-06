#define BATTERY_SERVICE_UUID "bbd16cf5-9234-43eb-93b5-679e4142f689"
#define BATTERY_CHARGE_UUID "12fb1e82-a141-429a-86c1-6be3f0c561af"

#define BATTERY_SEND_INTERVAL 10000 // ms

#define BATTERY_PIN 2 // ADC2

BLEService batteryService(BATTERY_SERVICE_UUID);
BLEUnsignedIntCharacteristic batteryChargeCharacteristic(BATTERY_CHARGE_UUID, BLERead | BLENotify);

unsigned long batterySendPreviousMillis = 0;

void setup_battery() {
    pinMode(BATTERY_PIN, INPUT);
    batteryService.addCharacteristic(batteryChargeCharacteristic);
    BLE.addService(batteryService);
}

void loop_battery() {

    unsigned long currentMillis = millis();

    if (currentMillis - batterySendPreviousMillis >= BATTERY_SEND_INTERVAL) {
        batterySendPreviousMillis = currentMillis;

        int batteryCharge = analogRead(BATTERY_PIN);
        batteryChargeCharacteristic.writeValue(batteryCharge);
    }

}