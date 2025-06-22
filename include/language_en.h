#pragma once

#include <pgmspace.h>

#define GET_TEXT_F(buffer, strg_ptr) (strcpy_P((buffer), (strg_ptr)), (buffer))

#define SPRINTF_FROM_PROGMEM(buffer, format_buffer, format_ptr, ...) ({ \
    strcpy_P((format_buffer), (format_ptr));                            \
    sprintf((buffer), (format_buffer), __VA_ARGS__);                    \
    (buffer);                                                           \
})

const char MSG_MAIN[] PROGMEM = "Main";
const char MSG_STATUS[] PROGMEM = "Status";
const char MSG_BUTTON_PRINT[] PROGMEM = "Print";
const char MSG_PREPARE[] PROGMEM = "Prepare";
const char MSG_CONTROL[] PROGMEM = "Control";
const char MSG_INFO_SCREEN[] PROGMEM ="Info Screen";
const char MSG_LEVEL_BED[] PROGMEM ="Level Bed";

const char RANGE_FORMAT[] PROGMEM  = "%3d/%3d";
const char PERCENT_FORMAT[] PROGMEM  = "%3d %%";
const char TIME_FORMAT[] PROGMEM  = "%2d:%2d";

