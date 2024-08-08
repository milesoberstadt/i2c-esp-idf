#include "Arduino.h"

#include <memory>

#include "layout/layout.h"
#include "layout/layout_devices.h"

#include "devices.h"
#include "ui.h"
#include "i2c/slave.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#error  "The current version is not supported for the time being, please use a version below Arduino ESP32 3.0"
#endif

Layout *layout;
Devices *devices;
I2CSlave *i2c_slave;
UI *ui;

void setup()
{

    Serial.begin(115200);
    Serial.println("Hello T-Display-S3");

    // init singletons
    ui = &UI::instance();
    i2c_slave = &I2CSlave::instance();
    
    // boot display
    ui->getGFX()->print("Hello T-Display-S3");

    // setup layout
    layout = new LayoutDevices();
    devices = &Devices::instance();
    devices->attach((LayoutDevices*)layout);
    layout->draw();
    
}

void loop(void)
{
 
    // delay(2000);

}