#ifndef __PRINTER_DATA_H
#define __PRINTER_DATA_H
#include <stdbool.h>
#include "utils/utils.h"

enum printer_state {
    STATE_OFFLINE = 0,
    STATE_ERROR,
    STATE_IDLE,
    STATE_PRINTING,
    STATE_PAUSED,
};

enum printer_features {
    FEATURE_RESTART           = BIT(0),
    FEATURE_FIRMWARE_RESTART  = BIT(1),
    FEATURE_HOME              = BIT(2),
    FEATURE_DISABLE_STEPPERS  = BIT(3),
    FEATURE_PAUSE             = BIT(4),
    FEATURE_RESUME            = BIT(5),
    FEATURE_STOP              = BIT(6),
    FEATURE_EMERGENCY_STOP    = BIT(7),
    FEATURE_EXTRUDE           = BIT(8),
    FEATURE_RETRACT           = BIT(9),
    FEATURE_IGNORE_ERROR      = BIT(10),
    FEATURE_CONTINUE_ERROR    = BIT(11),
    FEATURE_COOLDOWN          = BIT(12),
    FEATURE_RETRY_ERROR       = BIT(13),
};

enum printer_temp_device {
    TEMP_BED       = 0,
    TEMP_NOZZLE1   = 1,
    TEMP_CHAMBER   = 2,
};

struct _printer_data {
        union {
            struct {
                bool can_extrude : 1;
                bool homed_axis : 1;
                bool absolute_coords : 1;
            };
            unsigned char rawState;
        };
        enum printer_state state;
        char* state_message;
        char* popup_message;
        float temperatures[3];
        float target_temperatures[3];
        float position[3];
        float elapsed_time_s;
        float printed_time_s;
        float remaining_time_s;
        float filament_used_mm;
        char* print_filename; 
        float print_progress;
        float fan_speed;
        float speed_mult;
        float extrude_mult;
        int total_layers;
        int current_layer;
        float pressure_advance;
        float smooth_time;
        int feedrate_mm_per_s;
        enum printer_features error_screen_features;
};

typedef struct {
    enum printer_state state;
    float print_progress; // 0 -> 1
    unsigned int power_devices;
    bool success;
} printer_data_min;

#endif