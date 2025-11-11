#include "printer_driver_base.h"
#include "printer_data.h"
#include "esp_http_client.h"
#include "cJSON.h"

#define MAX_REQUEST_FAIL_COUNT 5

struct klipper_printer_info {
    enum printer_features supported_features;
    enum printer_temp_device supported_temp_devices;
    struct _printer_data printer_data;
    char printer_url[64];
};

static struct klipper_printer_info printer_info = {0};
static size_t req_fail_count = 0;

bool klipper_printer_init(char *ip, char *port)
{
    if (!ip || !port)
        return false;
    
    printer_info.supported_features = FEATURE_RESTART
        | FEATURE_FIRMWARE_RESTART
        | FEATURE_HOME
        | FEATURE_DISABLE_STEPPERS
        | FEATURE_PAUSE
        | FEATURE_RESUME
        | FEATURE_EMERGENCY_STOP
        | FEATURE_EXTRUDE
        | FEATURE_RETRACT
        | FEATURE_COOLDOWN ;
    
    printer_info.supported_temp_devices = TEMP_BED
        | TEMP_NOZZLE1;
    
    printer_info.printer_data.error_screen_features = FEATURE_RESTART 
        | FEATURE_FIRMWARE_RESTART;

    snprintf(printer_info.printer_url, sizeof(printer_info.printer_url),
        "http://%s:%s", ip, port);

    net_init_client(printer_info.printer_url);

    return true;
}

void klipper_parse_json_data(char *buf, size_t len) {
    if (!buf)
        return;

    cJSON *json = cJSON_ParseWithLength(buf, len);
    if (!json) {
        printf("Failed: %s\n", cJSON_GetErrorPtr());
        return;
    }

    char *str = cJSON_Print(json);
    printf("%s\n", str);

    cJSON_Delete(json);
}

void klipper_printer_fetch(void)
{
    char url[256];
    bool ret;

    snprintf(url, sizeof(url),
             "%s/printer/objects/query?extruder&heater_bed&toolhead&gcode_move&virtual_sdcard&print_stats&webhooks&fan&display_status",
             printer_info.printer_url);

    ret = net_req_http(url, klipper_parse_json_data);
    if (ret) {
        if (printer_info.printer_data.state == STATE_OFFLINE)
            printer_info.printer_data.state = STATE_ERROR;
        req_fail_count = 0;
    } else {
        req_fail_count++;
        if (req_fail_count >= MAX_REQUEST_FAIL_COUNT)
            printer_info.printer_data.state = STATE_OFFLINE;
    }

}

void klipper_printer_fetch_min(void)
{
    char url[256];
    bool ret;

    snprintf(url, sizeof(url),
             "%s/printer/objects/query?webhooks&print_stats&virtual_sdcard",
             printer_info.printer_url);

    ret = net_req_http(url, klipper_parse_json_data);
    if (ret)
        printer_info.printer_data.state = STATE_IDLE;
    else
        printer_info.printer_data.state = STATE_OFFLINE;
}

void klipper_printer_disconnect(void)
{
    printer_info.printer_data.state = STATE_OFFLINE;
}

bool klipper_printer_connection_test(void)
{
    char url[256];
    bool ret = false;

    snprintf(url, sizeof(url), "%s/printer/info",
             printer_info.printer_url);

    ret = net_req_http(url, klipper_parse_json_data);
    if (!ret) {
        return false;
    }

    return true;
}

enum printer_state klipper_printer_get_state(void)
{
    return printer_info.printer_data.state;
}

printer_ops_t printer_klipper = {
    .init = klipper_printer_init,
    .fetch = klipper_printer_fetch,
    .fetch_min = klipper_printer_fetch_min,
    .connection_test = klipper_printer_connection_test,
    .get_printer_state = klipper_printer_get_state,
    .disconnect = klipper_printer_disconnect,
};

