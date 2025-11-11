
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "display/dwin_lcd.h"
#include "common.h"
#include "networking/wifi_config.h"
#include "device_config.h"
#include "commands.h"
#include "printer_base.h"

TaskHandle_t fetch_task;

SemaphoreHandle_t wifi_connected_sem = NULL;
bool wifi_connected = false;

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
  nvs_init();
  wifi_connect(0, NULL);
  if (!printer_init()) {
    printf("failed to initize the printer\n");
  }

  screen_init();
  screen_setup();
  command_init();

  wifi_connected_sem = xSemaphoreCreateBinary();

  xTaskCreatePinnedToCore(
    priter_fetch_task,            /* Task function. */
    "Fetch Tasks",                 /* name of task. */
    4096,                    /* Stack size of task */
    NULL,                     /* parameter of the task */
    1,                        /* priority of the task */
    &fetch_task,                   /* Task handle to keep track of created task */
    1); 

  while (1) {
    commmand_run();
    vTaskDelay(pdMS_TO_TICKS(10));

    //TODO render tasks
  }
}