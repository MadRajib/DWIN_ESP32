#ifndef _DWIN_H
#define _DWIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int dwin_init(void);
bool dwin_handshake(void);
void dwin_send(void);
void dwin_add_byte(uint8_t bval);
void dwin_add_word(uint16_t bval);
void dwin_add_long(uint32_t lval);
void dwin_add_string(char * const string);

void dwin_update_lcd(void);
void dwin_write_big_endian_int64(uint16_t value);

#endif