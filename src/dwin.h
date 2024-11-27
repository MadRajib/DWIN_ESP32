#ifndef _DWIN_H
#define _DWIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int DWIN_init(void);

bool DWIN_Handshake(void);

void DWIN_Send(size_t *i);

void DWIN_Byte(size_t *i, const uint16_t bval);

#endif