#ifndef __PRINTER_DRIVER_BASE_H
#define __PRINTER_DRIVER_BASE_H
#include <stdbool.h>
#include "networking/net.h"
#include "networking/wifi_config.h"
#include "printer_data.h"

typedef struct {
    bool (*init)(char *url, char *port);
    void (*fetch)(void);
    void (*fetch_min)(void);
    bool (*connection_test)(void);
    enum printer_state (*get_printer_state)(void);
    void (*disconnect)(void);
    struct _printer_data * (*get_printer_data)(void); 
} printer_ops_t;

#endif