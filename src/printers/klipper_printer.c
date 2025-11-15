#include "printer_driver_base.h"
#include "printer_data.h"
#include "esp_http_client.h"
#include "utils/utils.h"

#define MAX_REQUEST_FAIL_COUNT 5

struct klipper_printer_info {
    enum printer_features supported_features;
    enum printer_temp_device supported_temp_devices;
    struct _printer_data printer_data;
    char printer_url[64];
};

static struct klipper_printer_info printer_info = {0};
static size_t req_fail_count = 0;

static enum printer_state map_state(const char *s)
{
    if (!s)
        return STATE_OFFLINE;

    if (!strcmp(s, "ready"))
        return STATE_IDLE;
    if (!strcmp(s, "standby"))
        return STATE_IDLE;
    if (!strcmp(s, "startup"))
        return STATE_IDLE;

    if (!strcmp(s, "printing"))
        return STATE_PRINTING;
    if (!strcmp(s, "paused"))
        return STATE_PAUSED;
    if (!strcmp(s, "error"))
        return STATE_ERROR;

    if (!strcmp(s, "complete"))
        return STATE_IDLE;
    if (!strcmp(s, "shutdown"))
        return STATE_OFFLINE;
    if (!strcmp(s, "offline"))
        return STATE_OFFLINE;

    return STATE_OFFLINE; 
}

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
    int count = 0;
    cJSON *json, *item, *extr, *bed, *th, *gm, *vsd, *ps, *fan;
    struct _printer_data *pd;
    if (!buf)
        return;
    
    pd  = &printer_info.printer_data;

    json = cJSON_ParseWithLength(buf, len);
    if (!json) {
        printf("Failed: %s\n", cJSON_GetErrorPtr());
        return;
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "result");
    if (!cJSON_IsObject(item))
        goto free_json;
    
    item = cJSON_GetObjectItemCaseSensitive(item, "status");
    if (!cJSON_IsObject(item))
        goto free_json;

    // ---------- EXTRUDER ----------
    extr = cJSON_GetObjectItemCaseSensitive(item, "extruder");
    if (extr) {
        pd->temperatures[TEMP_NOZZLE1]        = get_float(extr, "temperature", 0);
        pd->target_temperatures[TEMP_NOZZLE1] = get_float(extr, "target", 0);
        pd->pressure_advance       = get_float(extr, "pressure_advance", 0);
        pd->smooth_time            = get_float(extr, "smooth_time", 0);

        // boolean
        pd->can_extrude = get_bool(extr, "can_extrude", false);
    }

    // ---------- BED ----------
    bed = cJSON_GetObjectItemCaseSensitive(item, "heater_bed");
    if (bed) {
        pd->temperatures[TEMP_BED] = get_float(bed, "temperature", 0);
        pd->target_temperatures[TEMP_BED] = get_float(bed, "target", 0);
    }
    
    // ---------- TOOLHEAD ----------
    th = cJSON_GetObjectItemCaseSensitive(item, "toolhead");
    if (th) {
        cJSON *pos = cJSON_GetObjectItem(th, "position");
        get_array3(pos, pd->position);

        // homed axis is empty string when *not homed*
        char *homed = get_string(th, "homed_axes");
        pd->homed_axis = (homed && homed[0] != '\0');
        free(homed);
        homed = NULL;
    }

    // ---------- GCODE MOVE ----------
    gm = cJSON_GetObjectItem(item, "gcode_move");
    if (gm) {
        pd->absolute_coords = get_bool(gm, "absolute_coordinates", true);
        pd->speed_mult      = get_float(gm, "speed_factor", 1.0f);
        pd->extrude_mult    = get_float(gm, "extrude_factor", 1.0f);

        pd->feedrate_mm_per_s = (int)get_float(gm, "speed", 0);
    }

    // ---------- VIRTUAL SD CARD ----------
    vsd = cJSON_GetObjectItem(item, "virtual_sdcard");
    if (vsd) {
        pd->print_progress = get_float(vsd, "progress", 0);
    }

    // ---------- PRINT STATS ----------
    ps = cJSON_GetObjectItem(item, "print_stats");
    if (ps) {
        pd->print_filename  = get_string(ps, "filename");
        pd->printed_time_s  = get_float(ps, "print_duration", 0);
        pd->elapsed_time_s  = get_float(ps, "total_duration", 0);
        pd->filament_used_mm = get_float(ps, "filament_used", 0);

        cJSON *info = cJSON_GetObjectItem(ps, "info");
        if (info) {
            cJSON *tlay = cJSON_GetObjectItem(info, "total_layer");
            cJSON *clay = cJSON_GetObjectItem(info, "current_layer");

            pd->total_layers   = cJSON_IsNumber(tlay) ? tlay->valueint : -1;
            pd->current_layer  = cJSON_IsNumber(clay) ? clay->valueint : -1;
        }
    }

    // ---------- FAN ----------
    fan = cJSON_GetObjectItem(item, "fan");
    if (fan) {
        pd->fan_speed = get_float(fan, "speed", 0);
    }

    // ---------- WEBHOOKS ----------
    cJSON *webhooks = cJSON_GetObjectItem(item, "webhooks");
    pd->state_message = get_string(webhooks, "state_message");

    // state: "ready", "standby", etc.
    char *state_str = get_string(webhooks, "state");
    pd->state = map_state(state_str);
    free(state_str);
    
free_json:
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

struct _printer_data * klipper_get_printer_data(){
    return &printer_info.printer_data;
}

printer_ops_t printer_klipper = {
    .init = klipper_printer_init,
    .fetch = klipper_printer_fetch,
    .fetch_min = klipper_printer_fetch_min,
    .connection_test = klipper_printer_connection_test,
    .get_printer_state = klipper_printer_get_state,
    .disconnect = klipper_printer_disconnect,
    .get_printer_data = klipper_get_printer_data,
};

