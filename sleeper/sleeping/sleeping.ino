#include <Arduino.h>

#define SIGNAL_PIN D0
#define LED_PIN LED_BUILTIN

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(SIGNAL_PIN, INPUT);

  // interrupt for wakeup or sleep
  attachInterrupt(digitalPinToInterrupt(SIGNAL_PIN), handleSignalChange, CHANGE);
}

void loop() {

}

// Fonction appelée sur interruption
void handleSignalChange() {
  if (digitalRead(SIGNAL_PIN) == HIGH) {
    //digitalWrite(LED_PIN, LOW);
    wakeUp();
  } else {
    // Signal de mise en veille
    //digitalWrite(LED_PIN, HIGH);
    enterSleepMode();
  }
}


// Fonction de réveil
void wakeUp() {
  digitalWrite(LED_PIN, LOW);
}

// Fonction de mise en veille
void enterSleepMode() {
  digitalWrite(LED_PIN, HIGH);
  NRF_POWER->SYSTEMOFF = 1;
}