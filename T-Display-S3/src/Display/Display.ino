#include "Arduino.h"
#include "ui.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#error  "The current version is not supported for the time being, please use a version below Arduino ESP32 3.0"
#endif

void setup()
{

    Serial.begin(115200);
    Serial.println("Hello T-Display-S3");

    UI& ui = uiInstance();

    ui.getGFX()->print("Hello T-Display-S3");
    
}

void loop(void)
{
 
    delay(2000);

}