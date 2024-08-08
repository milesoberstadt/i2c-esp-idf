#define pin D0

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // make the pushbutton's pin an input:
  pinMode(pin, OUTPUT);
}

void loop() {
  digitalWrite(pin, LOW);
  Serial.println(digitalRead(pin));
  delay(3000);
  digitalWrite(pin, HIGH);
  Serial.println(digitalRead(pin));
  delay(3000);
}