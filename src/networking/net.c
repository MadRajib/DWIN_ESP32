#include "net.h"
#include <stdio.h>
#include "device_config.h"
#include "wifi_config.h"
#include "esp_http_client.h"
#include "cJSON.h"

#define MAX_HTTP_OUTPUT_BUFFER 2048

static esp_http_client_handle_t client = NULL;
static esp_http_client_config_t cfg;
char output_buffer[MAX_HTTP_OUTPUT_BUFFER + 1];

void net_init_client(char *url) {
    cfg.url = url;
    cfg.timeout_ms = 1000;

    if (!client) {
        client = esp_http_client_init(&cfg);
        if (!client) {
            printf("HTTP client init failed\n");
            return;
        }
    }
}

bool net_req_http(char *url, void (*parser)(char *buf, size_t len)) {
    esp_err_t err;
    int content_length;

    if (!url || !client)
        return false;

    memset(output_buffer, 0, sizeof(output_buffer));

    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    printf("url: %s", url);

    err = esp_http_client_open(client, 0);
    if (err == ESP_OK) {
        content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            printf("HTTP client fetch headers failed\n");
        } else {
            int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
            if (data_read >= 0) {
                if (parser)
                    parser(output_buffer, data_read);
                return true;
            } else {
                printf("Failed to read response\n");
            }
        }
    }

    return false;
}