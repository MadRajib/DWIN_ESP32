#include <Arduino.h>
#include "test/dwin_lcd_test.h"

Dwin_Lcd dwin_lcd;

void setup() {
    Serial.begin(115200);
    dwin_lcd.screen_setup();
}

void loop(){

}