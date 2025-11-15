#include "dwin_lcd.h"
#include <string.h>
#include <stdio.h>
#include "utils/language_en.h"
#include "dwin.h"

#define SPRINTF(buffer, format_buffer, ...) ({ \
    sprintf((buffer), (format_buffer), __VA_ARGS__);                    \
    (buffer);                                                           \
})

char str_buffer[20];
char formatBuffer[20];

static enum screen_win prev_win = NO_SCREEN;
static enum screen_win cur_win = NO_SCREEN;

/* Set display rotation */
void screen_set_display_rot(uint8_t rot) {
  dwin_add_byte(0x34);
  dwin_add_byte(0x5A);
  dwin_add_byte(0xA5);
  dwin_add_byte(rot);
  dwin_send();
}

/* Update screen buffer */
void screen_update(void) {
 dwin_update_lcd();
}

/*
  Draw an Icon
  libID: Icon library ID
  picID: Icon ID
  x/y: Upper-left point
*/ 
void screen_add_icon(uint8_t libID, uint8_t picID, uint16_t x, uint16_t y) {
  
  x = _MIN(x, DWIN_WIDTH - 1);
  y = _MIN(y, DWIN_HEIGHT - 1);
  dwin_add_byte(0x23);
  dwin_add_word(x);
  dwin_add_word(y);
  dwin_add_byte(0x80 | libID);
  dwin_add_byte(picID); 
  dwin_send();
}

/*
  Draw a rectangle
  mode: 0=frame, 1=fill, 2=XOR fill
  color: Rectangle color
  xStart/yStart: upper left point
  xEnd/yEnd: lower right point
*/
void screen_add_rect(uint8_t mode, uint16_t color,
                         uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd) {
  dwin_add_byte(0x05);
  dwin_add_byte(mode);
  dwin_add_word(color);
  dwin_add_word(xStart);
  dwin_add_word(yStart);
  dwin_add_word(xEnd);
  dwin_add_word(yEnd);
  dwin_send();
}

/*
 Draw a string
  widthAdjust: true=self-adjust character width; false=no adjustment
  bShow: true=display background color; false=don't display background color
  size: Font size
  color: Character color
  bColor: Background color
  x/y: Upper-left coordinate of the string
  *string: The string
*/
void screen_draw_string(bool widthAdjust, bool bShow, uint8_t size,
                      uint16_t color, uint16_t bColor, uint16_t x, uint16_t y, const char *string) {
  dwin_add_byte(0x11);
  /*
   Bit 7: widthAdjust
   Bit 6: bShow
   Bit 5-4: Unused (0)
   Bit 3-0: size
  */
  dwin_add_byte((widthAdjust * 0x80) | (bShow * 0x40) | size);
  dwin_add_word(color);
  dwin_add_word(bColor);
  dwin_add_word(x);
  dwin_add_word(y);
  dwin_add_string(string);
  dwin_send();
}

/*
 Draw a positive integer
  bShow: true=display background color; false=don't display background color
  zeroFill: true=zero fill; false=no zero fill
  zeroMode: 1=leading 0 displayed as 0; 0=leading 0 displayed as a space
  size: Font size
  color: Character color
  bColor: Background color
  iNum: Number of digits
  x/y: Upper-left coordinate
  value: Integer value
*/
void screen_draw_int(uint8_t bShow, bool zeroFill, uint8_t zeroMode, uint8_t size, uint16_t color,
                          uint16_t bColor, uint8_t iNum, uint16_t x, uint16_t y, uint16_t value) {
  dwin_add_byte(0x14);
  // Bit 7: bshow
  // Bit 6: 1 = signed; 0 = unsigned number;
  // Bit 5: zeroFill
  // Bit 4: zeroMode
  // Bit 3-0: size
  dwin_add_byte((bShow * 0x80) | (zeroFill * 0x20) | (zeroMode * 0x10) | size);
  dwin_add_word(color);
  dwin_add_word(bColor);
  dwin_add_byte(iNum);
  dwin_add_byte(0); // fNum
  dwin_add_word(x);
  dwin_add_word(y);

  dwin_write_big_endian_int64(value);

  dwin_send();
}

/* Initialize Progress Bar (optional) */
void screen_hmi_init() {
  for (uint16_t t = 0; t <= 100; t += 2) {
    screen_add_icon(ICON, ICON_Bar, 15, 260);
    screen_add_rect(1, Color_Bg_Black, 15 + t * 242 / 100, 260, 257, 280);
    screen_update();
  }
}

#if 0
void screen_update_status(void)
{
  uint16_t col = 340;
  screen_draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    50 , SPRINTF(str_buffer, RANGE_FORMAT, 999, 999));

  screen_draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    80, SPRINTF(str_buffer, RANGE_FORMAT, 0, 999));

  screen_draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    115, SPRINTF(str_buffer, PERCENT_FORMAT, 999));

  screen_draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    145, SPRINTF(str_buffer, PERCENT_FORMAT, 999));

  // draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 40 , 170,  999);
  // draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 40,  200,  999);
  // draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 40 , 230,  999);

  screen_draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 20 , 175,  999);
  screen_draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 90,  175,  999);
  screen_draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 20 , 205,  999);

  screen_draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    240, SPRINTF(str_buffer, TIME_FORMAT, 10, 12));

  screen_update();
}
#endif

void screen_update_status(struct _printer_data *printer_data)
{
  uint16_t col = 340;
  screen_draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    50 , SPRINTF(str_buffer, RANGE_FORMAT, 999, 999));

  screen_draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    80, SPRINTF(str_buffer, RANGE_FORMAT, 0, 999));

  screen_draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    115, SPRINTF(str_buffer, PERCENT_FORMAT, 999));

  screen_draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    145, SPRINTF(str_buffer, PERCENT_FORMAT, 999));

  // draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 40 , 170,  999);
  // draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 40,  200,  999);
  // draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 40 , 230,  999);

  screen_draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 20 , 175,  999);
  screen_draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 90,  175,  999);
  screen_draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 20 , 205,  999);

  screen_draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    240, SPRINTF(str_buffer, TIME_FORMAT, 10, 12));

  screen_update();
}

void screen_draw_status_frame() {
  uint16_t col = 310;
  uint16_t icon_off = col + 20;

  screen_add_rect(1, Color_Bg_Orange, col, 0, DWIN_WIDTH - 1, 30);
  screen_draw_string(false, false, DWIN_FONT_HEAD, Color_White, Color_Bg_Orange, col + 20, 4, MSG_STATUS);

  screen_add_rect(1, Color_Bg_Black, col,  31, DWIN_WIDTH, DWIN_HEIGHT - 1);

  /* Hot end */
  screen_add_icon(ICON, ICON_HotendTemp, icon_off, 50);

  /* Heat bed */
  screen_add_icon(ICON, ICON_BedTemp, icon_off, 80);

  screen_add_icon(ICON, ICON_Speed, icon_off, 115);
  screen_add_icon(ICON, ICON_FanSpeed, icon_off, 145);

  screen_add_icon(ICON, ICON_StepX,  icon_off, 175);
  screen_add_icon(ICON, ICON_StepY,  icon_off + 70, 175);
  screen_add_icon(ICON, ICON_StepZ,  icon_off, 205);

  screen_add_icon(ICON, ICON_PrintTime,  icon_off, 240);

  screen_update();
}

void screen_draw_print_icon(void) {
  screen_add_icon(ICON, ICON_Print_0, 10, 40);
  screen_draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 45, 108, MSG_BUTTON_PRINT);
  // screen_add_rect(0, Color_White, 10, 40, 119, 139);
}

void screen_draw_main_frame() {
  uint16_t col = 360;
  /* Reset Frame */
  screen_add_rect(1, Color_Bg_Green , 0, 0, col, 30);
  screen_add_rect(1, Color_Bg_Black, 0, 31, col, DWIN_WIDTH);

  screen_draw_string(false, false, DWIN_FONT_HEAD, Color_White, Color_Bg_Green, 14, 4, MSG_MAIN);
  screen_add_icon(ICON, ICON_Print_0, 30, 40);
  screen_draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 65, 108, MSG_BUTTON_PRINT);

  screen_add_icon(ICON, ICON_Prepare_0, 166, 40);
  screen_draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 196, 108, MSG_PREPARE);

  // screen_add_icon(ICON, ICON_Leveling_0, 246, 40);
  // screen_draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 276, 108, MSG_PREPARE);

  screen_add_icon(ICON, ICON_Control_0, 30, 160);
  screen_draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 60, 230, MSG_CONTROL);

  screen_add_icon(ICON, ICON_Info_0, 166, 160);
  screen_draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 185, 230, MSG_LEVEL_BED);

  // screen_add_icon(ICON, ICON_Prepare_0, 246, 160);
  // screen_draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 276, 230, MSG_PREPARE);

  screen_update();
}

void screen_draw_main_menu()
{
    screen_draw_main_frame();
    screen_draw_status_frame();
    // screen_update_status();
}

void screen_draw_blank_frame(enum error_codes code) {
  screen_add_rect(1, Color_Bg_Orange, 0, 0, DWIN_WIDTH, 30);
  screen_add_rect(1, Color_Bg_Black, 0, 31, DWIN_WIDTH, DWIN_HEIGHT);

  if (code == ERROR_WIFI_DISCONNECTED)
    screen_draw_string(false, false, DWIN_FONT_HEAD, Color_White, Color_Bg_Black, DWIN_HEIGHT / 2, 100, "Wifi Not Connected!");
  else if (code == ERROR_PRINTER_NOT_CONNECTED)
     screen_draw_string(false, false, DWIN_FONT_HEAD, Color_White, Color_Bg_Black, DWIN_HEIGHT / 2, 100, "Printer Not Connected!");
  screen_update();
}

void screen_init() {
  dwin_init();
  if (!dwin_handshake()) {
    printf("Connection timeout!\n");
    return;
  }
  
  printf("Connection Successfull\n");

  screen_set_display_rot(0);
  screen_update();
  screen_hmi_init();

  screen_switch(ERROR_SCREEN, ERROR_BLANK);
}

void screen_render(struct _printer_data *printer_data) {
    // screen_draw_main_frame();
    // screen_draw_status_frame();
    // screen_update_status();

    // screen_draw_blank_frame(WIFI_DISCONNECTED);
    // screen_draw_blank_frame(ERROR_WIFI_DISCONNECTED);
    if (cur_win == MAIN_SCREEN)
      screen_update_status(printer_data);
}

void screen_switch(enum screen_win win, enum error_codes error) {
  prev_win = cur_win;
  cur_win = win;

  switch (win) {
    case ERROR_SCREEN:
      screen_draw_blank_frame(error);
      break;
    case MAIN_SCREEN:
      screen_draw_main_menu();
      break;
    default:
  }
}