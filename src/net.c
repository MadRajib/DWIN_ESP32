#include "net.h"
#include <stdio.h>
#include "device_config.h"
#include "wifi_config.h"
#include "esp_http_client.h"

static bool printer_connected = false;

bool printer_connect(void) {
    char ip[16] = {0};
    char port[5] = {0};
    char url[64];

    if (!is_device_conf_set(CONF_PRINTER_IP) || !is_device_conf_set(CONF_PRINTER_PORT)) {
        printf("print ip or port is not set\n");
        return false;
    }

    printf("%s called\n", __func__);
    conf_get(CONF_PRINTER_IP, ip);
    conf_get(CONF_PRINTER_PORT, port);

    snprintf(url, sizeof(url), "http://%s:%s/", ip, port);

    esp_http_client_config_t cfg = {
        .url = url,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        printf("connected to printer\n");
        printer_connected = true;
        return true;
    } else {
        printf("printer no connected\n");
        printer_connected = false;
    }

    return false;
}

void printer_update() {
    if (!printer_connected)
        return;
}