#ifndef _DWIN_H
#define _DWIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

int DWIN_init(void);

bool DWIN_handshake(void);

void DWIN_send(void);

void DWIN_add_byte(const uint8_t bval);
void DWIN_add_word(const uint16_t bval);
void DWIN_add_long(const uint32_t lval);
void DWIN_add_string(const char * const string);
void DWIN_add_icon(uint8_t libID, uint8_t picID, uint16_t x, uint16_t y);
void DWIN_add_rect(uint8_t mode, uint16_t color,
                    uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd);
void DWIN_draw_string(bool widthAdjust, bool bShow, uint8_t size,
                      uint16_t color, uint16_t bColor, uint16_t x, uint16_t y, char *string);
void DWIN_draw_int(uint8_t bShow, bool zeroFill, uint8_t zeroMode, uint8_t size, uint16_t color,
                          uint16_t bColor, uint8_t iNum, uint16_t x, uint16_t y, uint16_t value);

#endif