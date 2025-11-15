#ifndef __PRINTER_BASE_H
#define __PRINTER_BASE_H

bool printer_init(void);
bool printer_connect(void);
void priter_fetch_task(void *params);
void printer_screen_render(void);
struct _printer_data * printer_get_render_data(void);

#endif