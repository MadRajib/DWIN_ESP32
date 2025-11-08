#ifndef __DEVICE_CONFIG_H
#define __DEVICE_CONFIG_H

#include "utils.h"
#include <stdbool.h>

enum CONF {
    CONF_SSID,
    CONF_PASS,
    CONF_PRINTER_IP,
    CONF_PRINTER_PORT,
    CONF_ALL
};

int conf_set(enum CONF type, const char *val);
int conf_erase(enum CONF type);
int conf_get(enum CONF type, char *conatiner);
void conf_save(void);
void conf_init_nvs(void);
bool is_device_conf_set(enum CONF type);

#endif