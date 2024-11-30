#include "dwin_lcd.h"
#include "esp_log.h"

static const char* TAG = "ESP";

void app_main()
{
    DWIN_init();

    if (DWIN_Handshake()) 
        ESP_LOGW(TAG, "Connection Successfull\n");
    else
        ESP_LOGW(TAG, "Connection timeout!\n");
}