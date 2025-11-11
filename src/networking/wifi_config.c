#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "string.h"
#include "wifi_config.h"
#include "device_config.h"

static int s_retry_num = 0;
bool is_sta_connected = false;
extern SemaphoreHandle_t wifi_connected_sem;
extern bool wifi_connected;

/* Wifi handles*/
// STA event handler
static void sta_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        printf("trying to connect\n");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        is_sta_connected = false;
        if (s_retry_num < 20)
        {
            esp_wifi_connect();
            s_retry_num++;
            printf("retry to connect to the AP\n");
        }

        printf("connect to the AP fail\n");
        // xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        printf("taking semaphore\n");
        wifi_connected = false;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        printf("got ip:" IPSTR "\n", IP2STR(&event->ip_info.ip));

        s_retry_num = 0;
        is_sta_connected = true;
        // xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        wifi_connected = true;
        xSemaphoreGive(wifi_connected_sem);
    }
}

void wifi_init(const int mode)
{

    wifi_config_t wifi_config;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    char tmp[65] = {0};
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if (mode == WIFI_AP)
        esp_netif_create_default_wifi_ap();
    else
        esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    memset(&wifi_config, 0, sizeof(wifi_config));
    
    switch(mode) {
        case WIFI_AP:      
            break;
        case WIFI_STA:
            ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_event_handler, NULL));
            ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_event_handler, NULL));
            
            conf_get(CONF_SSID, tmp);
            memcpy(wifi_config.sta.ssid, tmp, strlen(tmp));
            memset(tmp, 0, sizeof(tmp));
            conf_get(CONF_PASS, tmp);
            memcpy(wifi_config.sta.password, tmp, strlen(tmp));

            wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
            wifi_config.sta.pmf_cfg.capable = true;
            wifi_config.sta.pmf_cfg.required = false;

            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
            break;
    }
    
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}