#include <stdbool.h>
#include <stddef.h>
#include "printer_base.h"
#include "printers/printer_driver_base.h"
#include "device_config.h"
#include "display/dwin_lcd.h"

printer_ops_t *printer_ops = NULL;
static bool printer_initialized = false;

extern SemaphoreHandle_t wifi_connected_sem;
extern bool wifi_connected;

bool printer_init(void)
{
#if defined(PRINTER_KLIPPER)
    extern printer_ops_t printer_klipper;
    printer_ops = &printer_klipper;
#else
#error "No printer type defined"
#endif

    char ip[16] = {0};
    char port[5] = {0};

    if (!is_device_conf_set(CONF_PRINTER_IP) || !is_device_conf_set(CONF_PRINTER_PORT)) {
        printf("print ip or port is not set\n");
        return false;
    }

    conf_get(CONF_PRINTER_IP, ip);
    conf_get(CONF_PRINTER_PORT, port);

    if (!printer_ops->init(ip, port)) {
        printf("ERROR: %s : Failed to initialize printer\n", __func__);
        return false;
    }

    printer_initialized = true;

    return false;
}

bool printer_connect(void) {
     if (!printer_initialized) {
        printer_init();
        return false;
    }

    return printer_ops->connection_test();
}

static void printer_update(void) {

    if (printer_ops->get_printer_state() == STATE_OFFLINE) {
        if (!printer_connect()) {
            printf("failed to connect to printer\n");
            return;
        }

        printf("connected to printer\n");
        screen_switch(MAIN_SCREEN, ERROR_NONE);
    }

    printer_ops->fetch();
    screen_update_status(printer_ops->get_printer_data());
}

struct _printer_data * printer_get_render_data(void)
{
    return printer_ops->get_printer_data();
}

void priter_fetch_task(void *params) {

    (void)params;
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    for (;;) {
        /* Wait for wifi connection */
        if (xSemaphoreTake(wifi_connected_sem, portMAX_DELAY) == pdTRUE) {
            /* fetch */
            while (1) {
                if (wifi_connected) {
                    printer_update();
                    vTaskDelay(xDelay);
                } else {
                    break;
                }
            }
        }
        
    }
}
