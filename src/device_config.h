#ifndef __DEVICE_CONFIG_H
#define __DEVICE_CONFIG_H

#include "utils.h"

enum CONF {
    CONF_SSID,
    CONF_PASS,
    CONF_ALL
};

int conf_set(enum CONF type, const char *val);
int conf_erase(enum CONF type);
int conf_get(enum CONF type, char *conatiner);
int conf_is_wifi_set(void);
void conf_save(void);
void conf_init_nvs(void);

#endif