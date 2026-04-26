#ifndef _DWIN_H
#define _DWIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int DWIN_init(void);

bool DWIN_Handshake(void);

void DWIN_Send(void);

void DWIN_Byte(const uint8_t bval);

#endif