#include "string.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "device_config.h"

#define WIFI_SSID_LEN   33
#define WIFI_PASS_LEN   65
#define PRINTER_IP_LEN  15
#define PRINTER_PORT_LEN  5

struct device_config {
    char ssid[WIFI_SSID_LEN];
    char pass[WIFI_PASS_LEN];
    char printer_ip[PRINTER_IP_LEN];
    char printer_port[PRINTER_PORT_LEN];
    uint64_t modified;
};

static struct device_config g_config = {0};

static void read_string_from_nvs()
{
    nvs_handle_t handle;
    size_t length = WIFI_PASS_LEN;

    esp_err_t ret = nvs_open("storage", NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        LOGF("Error opening NVS (%s)\n", esp_err_to_name(ret));
        return;
    }

    ret = nvs_get_str(handle, "ssid", g_config.ssid, &length);
    if (ret != ESP_OK) {
        LOGF("Error reading string: %s\n", esp_err_to_name(ret));
        goto close;  
    }

    length = WIFI_PASS_LEN;
    ret = nvs_get_str(handle, "pass", g_config.pass, &length);
    if (ret != ESP_OK)
        LOGF("Error reading string: %s\n", esp_err_to_name(ret));

    length = PRINTER_IP_LEN;
    ret = nvs_get_str(handle, "p_ip", g_config.printer_ip, &length);
    if (ret != ESP_OK)
        LOGF("Error reading string: %s\n", esp_err_to_name(ret));
    
    length = PRINTER_PORT_LEN;
    ret = nvs_get_str(handle, "p_port", g_config.printer_port, &length);
    if (ret != ESP_OK)
        LOGF("Error reading string: %s\n", esp_err_to_name(ret));
close:
    nvs_close(handle);
}

static void write_string_to_nvs()
{
    // Always initialize NVS first
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();    // Erase if needed
        nvs_flash_init();
    }

    nvs_handle_t handle;
    ret = nvs_open("storage", NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        LOGF("Error opening NVS (%s)\n", esp_err_to_name(ret));
        return;
    }

    if (g_config.modified & (1 << CONF_SSID)) {
        // Write string
        ret = nvs_set_str(handle, "ssid", g_config.ssid);
        if (ret != ESP_OK) {
            LOGF("Error setting string (%s)\n", esp_err_to_name(ret));
        }
    }
    
    if (g_config.modified & (1 << CONF_PASS)) {
        ret = nvs_set_str(handle, "pass", g_config.pass);
        if (ret != ESP_OK) {
            LOGF("Error setting string (%s)\n", esp_err_to_name(ret));
        }
    }

    if (g_config.modified & (1 << CONF_PRINTER_IP)) {
        ret = nvs_set_str(handle, "p_ip", g_config.printer_ip);
        if (ret != ESP_OK) {
            LOGF("Error setting string (%s)\n", esp_err_to_name(ret));
        }
    }

    if (g_config.modified & (1 << CONF_PRINTER_PORT)) {
        ret = nvs_set_str(handle, "p_port", g_config.printer_port);
        if (ret != ESP_OK) {
            LOGF("Error setting string (%s)\n", esp_err_to_name(ret));
        }
    }

    // Commit
    nvs_commit(handle);
    nvs_close(handle);
}

int conf_erase(enum CONF type)
{
    switch (type) {
    case CONF_SSID:
        memset(g_config.ssid, 0, WIFI_SSID_LEN);
        break;
    case CONF_PASS:
        memset(g_config.pass, 0, WIFI_PASS_LEN);
        break;
    case CONF_PRINTER_IP:
        memset(g_config.printer_ip, 0, PRINTER_IP_LEN);
        break;
    case CONF_PRINTER_PORT:
        memset(g_config.printer_port, 0, PRINTER_PORT_LEN);
        break;
    case CONF_ALL:
        memset(&g_config, 0, sizeof(struct device_config));
        break;
    default:
        return -1;
    }   
    
    return 0;
}

int conf_set(enum CONF type, const char *val)
{
    switch (type) {
    case CONF_SSID:
        g_config.modified |= 1 << CONF_SSID;
        strncpy(g_config.ssid, val, WIFI_SSID_LEN - 1);
        break;
    case CONF_PASS:
        g_config.modified |= 1 << CONF_PASS;
        strncpy(g_config.pass, val, WIFI_PASS_LEN - 1);
        break;
    case CONF_PRINTER_IP:
        g_config.modified |= 1 << CONF_PRINTER_IP;
        strncpy(g_config.printer_ip, val, PRINTER_IP_LEN - 1);
        break;
    case CONF_PRINTER_PORT:
        g_config.modified |= 1 << CONF_PRINTER_PORT;
        strncpy(g_config.printer_port, val, PRINTER_PORT_LEN - 1);
        break;
    default:
        return -1;
    }   

    return 0;
}

int conf_get(enum CONF type, char *conatiner)
{
    if (!conatiner)
        return -1;

    switch (type) {
    case CONF_SSID:
        strncpy(conatiner, g_config.ssid, WIFI_SSID_LEN - 1);
        break;
    case CONF_PASS:
        strncpy(conatiner, g_config.pass, WIFI_PASS_LEN - 1);
        break;
    case CONF_PRINTER_IP:
        strncpy(conatiner, g_config.printer_ip, PRINTER_IP_LEN - 1);
        break;
    case CONF_PRINTER_PORT:
        strncpy(conatiner, g_config.printer_port, PRINTER_PORT_LEN - 1);
        break;
    default:
        return -1;
    }
    
    return 0;
}

void conf_init_nvs()
{
    read_string_from_nvs();
}

void conf_save()
{
    write_string_to_nvs();
}

bool is_device_conf_set(enum CONF type) {
    switch (type) {
        case CONF_SSID:
            return g_config.ssid[0] == '\0';
        case CONF_PASS:
            return g_config.pass[0] == '\0';
        case CONF_PRINTER_IP:
            return g_config.printer_ip[0] == '\0';
        case CONF_PRINTER_PORT:
            return g_config.printer_port[0] == '\0';
        default:
            return false;
    }

    return false;
}