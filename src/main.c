
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "dwin_lcd.h"
#include "wifi_config.h"
#include "device_config.h"
#include "net.h"
#include "commands.h"

void nvs_init() {
  esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    conf_init_nvs();
}

void app_main()
{
  bool printer_connected = false;
  nvs_init();
  wifi_connect(0, NULL);

  screen_init();
  screen_setup();
  command_init();


  EventGroupHandle_t wifi_group = wifi_get_event_group();

  while (1) {
    commmand_run();
    vTaskDelay(pdMS_TO_TICKS(10));

    EventBits_t bits = xEventGroupGetBits(wifi_group);
    if ((bits & BIT0) && !printer_connected) {
            printer_connect();
            printer_connected = true;
      }
    }

}