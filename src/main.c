#include "dwin.h"
#include "esp_log.h"
#include "dwin_lcd.h"

static const char* TAG = "ESP";

void app_main()
{
    dwin_init();

    if (dwin_handshake()) 
        ESP_LOGW(TAG, "Connection Successfull\n");
    else
        ESP_LOGW(TAG, "Connection timeout!\n");

    screen_setup();
}