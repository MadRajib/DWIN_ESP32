#include "dwin.h"
#include "encoder.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "ESP";

void app_main()
{
    DWIN_init();

    if (DWIN_Handshake()) { 
        ESP_LOGW(TAG, "Connection Successfull\n");
    }
    else {
        ESP_LOGW(TAG, "Connection timeout!\n");
        return;
    }

    encoder_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "System running");
    }
}