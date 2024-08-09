#define LED_PIN     D1
#define NUM_LEDS    2

#define LED_BRIGHTNESS  20

#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

unsigned int isBlinking = 0;
unsigned int blinkLedIndex;
unsigned int blinkRed;
unsigned int blinkGreen;
unsigned int blinkBlue;
unsigned long blinkOnDuration;
unsigned long blinkOffDuration;
unsigned long blinkStartTime;

void led_setup() {
    pixels.begin();
    for (int i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, 0);
    }
    pixels.show(); // Initialize all pixels to off
    pixels.setBrightness(LED_BRIGHTNESS);
}

void led_loop() {
    if (isBlinking) {
        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - blinkStartTime;

        if (elapsedTime < blinkOnDuration) {
            pixels.setPixelColor(blinkLedIndex, pixels.Color(blinkRed, blinkGreen, blinkBlue));
            pixels.show();

        } else if (elapsedTime < blinkOnDuration + blinkOffDuration) {
            pixels.setPixelColor(blinkLedIndex, pixels.Color(0, 0, 0));
            pixels.show();

        } else {
            blinkStartTime = currentTime;
        }

    }

    // Add any necessary code for continuous LED control here
}

void set_led_color(int ledIndex, int red, int green, int blue) {
    pixels.setPixelColor(ledIndex, pixels.Color(red, green, blue));
    pixels.show();
}

void turn_on_led(int ledIndex) {
    pixels.setPixelColor(ledIndex, pixels.Color(255, 255, 255)); // Turn on LED with white color
    pixels.show();
}

void turn_off_led(int ledIndex) {
    pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 0)); // Turn off LED
    pixels.show();
}

void start_led_blink(   unsigned int ledIndex, 
                        unsigned int red, 
                        unsigned int green, 
                        unsigned int blue, 
                        unsigned long onDuration, 
                        unsigned long offDuration) {
    isBlinking = true;
    blinkLedIndex = ledIndex;
    blinkRed = red;
    blinkGreen = green;
    blinkBlue = blue;
    blinkOnDuration = onDuration;
    blinkOffDuration = offDuration;
    blinkStartTime = millis();
}

void stop_led_blink(unsigned int ledIndex) {
    if (isBlinking && blinkLedIndex == ledIndex) {
        isBlinking = false;
        pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 0));
        pixels.show();
    }
}