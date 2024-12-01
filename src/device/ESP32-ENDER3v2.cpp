#include "../conf/global_config.h"
#include "lvgl.h"
#include "../core/lv_setup.h"
#include "screen_driver.h"
#include "dwin_lcd.h"
#include <Arduino.h>
#include "dwin.h"

void screen_setBrightness(unsigned char brightness)
{
}

void screen_setup()
{
    delay(800);
    while(!DWIN_Handshake()) {
        Serial.println("No ack from LCD Retrying!\n");
    }
    
    Serial.println("DWIN LCD connected\n");
    DWIN_Frame_SetDir(0);

    DWIN_UpdateLCD();

    // Encoder_Configuration();
    HMI_Init();
    HMI_StartFrame(true);
}
