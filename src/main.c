#include "dwin.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "dwin_lcd.h"
#include "wifi_config.h"
#include "device_config.h"

static const char* TAG = "ESP";

int wifi_connect(int argc, char **argv)
{
  if(conf_is_wifi_set()) {
    // PRINT("WIFI config not set!\n");
    return -1;
  }

  wifi_init(WIFI_STA);

  return 0;
}

void app_main()
{
    /* esp */
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    // conf_init_nvs();
    
    wifi_connect(0, NULL);

    dwin_init();

    if (dwin_handshake()) 
        ESP_LOGW(TAG, "Connection Successfull\n");
    else
        ESP_LOGW(TAG, "Connection timeout!\n");

    screen_setup();
}