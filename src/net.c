#include "net.h"
#include <stdio.h>
#include "device_config.h"

void printer_connect(void) {
    if (!is_device_conf_set(CONF_PRINTER_IP) || !is_device_conf_set(CONF_PRINTER_PORT)) {
        printf("print ip or port is not set\n");
        return;
    }
}