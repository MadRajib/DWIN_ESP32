#include <Arduino.h>
#include "conf/global_config.h"
#include "serial/serial_console.h"
#include "wifi/wifi_setup.h"
#include "wifi/ip_setup.h"
#include "core/data_setup.h"
#include "core/lv_setup.h"
#include "device/screen_driver.h"

void setup() {
    Serial.begin(115200);
    serial_console::greet();
    load_global_config();
    screen_setup();
    // lv_setup();
    // LOG_LN("Screen init done");
    
    wifi_init();
    // ota_init();
    ip_init();
    data_setup();

    // nav_style_setup();
    // main_ui_setup();
}

void loop(){
    wifi_ok();
    data_loop();
    lv_handler();
    serial_console::run();

    // if (is_ready_for_ota_update())
    // {
    //     ota_do_update();
    // }

}