#ifndef __WIFI_CONFIG_H
#define __WIFI_CONFIG_H
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "utils/utils.h"

enum { WIFI_AP, WIFI_STA };

void wifi_init(const int);

#endif