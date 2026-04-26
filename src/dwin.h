#ifndef _DWIN_H
#define _DWIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int DWIN_init(void);

bool DWIN_handshake(void);

void DWIN_send(void);

void DWIN_add_byte(const uint8_t bval);

#endif