#include "dwin_lcd_test.h"
#include "../include/language_en.h"

void Dwin_Lcd::add_byte(uint8_t bval) {
  this->send_buf[this->buf_index++] = bval;
}

void Dwin_Lcd::add_word(uint16_t bval) {
  this->send_buf[this->buf_index++] = bval >> 8;
  this->send_buf[this->buf_index++] = bval & 0xFF;
}

void Dwin_Lcd::add_long(uint32_t lval) {
  this->send_buf[this->buf_index++] = (lval >> 24) & 0xFF;
  this->send_buf[this->buf_index++] = (lval >> 16) & 0xFF;
  this->send_buf[this->buf_index++] = (lval >>  8) & 0xFF;
  this->send_buf[this->buf_index++] = lval & 0xFF;
}

void Dwin_Lcd::add_string(char * const string) {
  const size_t len = _MIN(sizeof(send_buf) - this->buf_index, strlen(string)) + 1;
  memcpy(&send_buf[this->buf_index], string, len);
  this->buf_index += len;
}

/* Send data buf to display buffer */
void Dwin_Lcd::send(void) {

  for (int i = 0; i < this->buf_index; i++) {
    LCD_SERIAL_PORT.write(this->send_buf[i]);
    delayMicroseconds(1);
  }

  for (int i = 0; i < 4; i++) {
    LCD_SERIAL_PORT.write(this->send_buf_tail[i]);
    delayMicroseconds(1);
  }

  /* Reset buffer index */
  this->buf_index = 1;
}

bool Dwin_Lcd::handshake(void)
{ 
  LCD_SERIAL_PORT.begin(LCD_BAUDRATE, SERIAL_8N1, 16, 17);

  const uint32_t serial_connect_timeout = millis() + 1000UL;
  while (!LCD_SERIAL_PORT.available() && PENDING(millis(), serial_connect_timeout)) { /*nada*/ }
  delay(10);

  this->add_byte(0x00);
  this->send();

  delay(20);

  size_t recnum = 0;

  while (LCD_SERIAL_PORT.available() > 0 && recnum < (signed)sizeof(this->read_buf)) {
    this->read_buf[recnum] = LCD_SERIAL_PORT.read();
  
    if (read_buf[0] != FHONE) {
      if (recnum > 0) {
        recnum = 0;
        memset(read_buf, 0, sizeof(read_buf));
      }
      continue;
    }
    delay(10);
    recnum++;
  }

  return ( recnum >= 3
        && read_buf[0] == FHONE
        && read_buf[1] == '\0'
        && read_buf[2] == 'O'
        && read_buf[3] == 'K' );

  return false;
}

void Dwin_Lcd::screen_setup(void) {
    delay(800);
    while(!handshake()) {
        Serial.println("No ack from LCD Retrying!\n");
    }
    
    Serial.println("DWIN LCD connected\n");
    set_display_rot(0);

    update_lcd();

    hmi_init();
    draw_main_frame();
}

/* Set display rotation */
void Dwin_Lcd::set_display_rot(uint8_t rot) {
  add_byte(0x34);
  add_byte(0x5A);
  add_byte(0xA5);
  add_byte(rot);
  send();
}

/* Update screen buffer */
void Dwin_Lcd::update_lcd(void) {
  add_byte(0x3D);
  send();
}

/*
  Draw an Icon
  libID: Icon library ID
  picID: Icon ID
  x/y: Upper-left point
*/ 
void Dwin_Lcd::add_icon(uint8_t libID, uint8_t picID, uint16_t x, uint16_t y) {
  
  x = _MIN(x, DWIN_WIDTH - 1);
  y = _MIN(y, DWIN_HEIGHT - 1);
  add_byte(0x23);
  add_word(x);
  add_word(y);
  add_byte(0x80 | libID);
  add_byte(picID); 
  send();
}

/*
  Draw a rectangle
  mode: 0=frame, 1=fill, 2=XOR fill
  color: Rectangle color
  xStart/yStart: upper left point
  xEnd/yEnd: lower right point
*/
void Dwin_Lcd::add_rect(uint8_t mode, uint16_t color,
                         uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd) {
  add_byte(0x05);
  add_byte(mode);
  add_word(color);
  add_word(xStart);
  add_word(yStart);
  add_word(xEnd);
  add_word(yEnd);
  send();
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
void Dwin_Lcd::draw_string(bool widthAdjust, bool bShow, uint8_t size,
                      uint16_t color, uint16_t bColor, uint16_t x, uint16_t y, char *string) {
  add_byte(0x11);
  /*
   Bit 7: widthAdjust
   Bit 6: bShow
   Bit 5-4: Unused (0)
   Bit 3-0: size
  */
  add_byte((widthAdjust * 0x80) | (bShow * 0x40) | size);
  add_word(color);
  add_word(bColor);
  add_word(x);
  add_word(y);
  add_string(string);
  send();
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
void Dwin_Lcd::draw_int(uint8_t bShow, bool zeroFill, uint8_t zeroMode, uint8_t size, uint16_t color,
                          uint16_t bColor, uint8_t iNum, uint16_t x, uint16_t y, uint16_t value) {
  add_byte(0x14);
  // Bit 7: bshow
  // Bit 6: 1 = signed; 0 = unsigned number;
  // Bit 5: zeroFill
  // Bit 4: zeroMode
  // Bit 3-0: size
  add_byte((bShow * 0x80) | (zeroFill * 0x20) | (zeroMode * 0x10) | size);
  add_word(color);
  add_word(bColor);
  add_byte(iNum);
  add_byte(0); // fNum
  add_word(x);
  add_word(y);

  // Write a big-endian 64 bit integer
  const size_t p = this->buf_index;
  for (char count = 8; count--;) { // 7..0
    ++this->buf_index;
    send_buf[p + count] = value;
    value >>= 8;
  }

  send();
}

/* Initialize Progress Bar (optional) */
void Dwin_Lcd::hmi_init() {
  for (uint16_t t = 0; t <= 100; t += 2) {
    add_icon(ICON, ICON_Bar, 15, 260);
    add_rect(1, Color_Bg_Black, 15 + t * 242 / 100, 260, 257, 280);
    update_lcd();
    delay(20);
  }
}

void Dwin_Lcd::update_status(void)
{
  uint16_t col = 360;
  draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    50 , SPRINTF_FROM_PROGMEM(str_buffer, formatBuffer, RANGE_FORMAT, 999, 999));

  draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    80, SPRINTF_FROM_PROGMEM(str_buffer, formatBuffer, RANGE_FORMAT, 0, 999));

  draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    110, SPRINTF_FROM_PROGMEM(str_buffer, formatBuffer, PERCENT_FORMAT, 999));

  draw_string(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, col + 40,
    140, SPRINTF_FROM_PROGMEM(str_buffer, formatBuffer, PERCENT_FORMAT, 999));

  draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 5, 250,  999);
  draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 45, 250,  999);
  draw_int(true, true, 0, DWIN_FONT_STAT, Color_White, Color_Bg_Black, 3, col + 85, 250,  999);

  update_lcd();
}

void Dwin_Lcd::draw_status_frame() {
  uint16_t col = 360;

  add_rect(1, Color_Bg_Orange, col, 0, DWIN_WIDTH - 1, 30);
  draw_string(false, false, DWIN_FONT_HEAD, Color_White, Color_Bg_Orange, col + 20, 4, GET_TEXT_F(str_buffer, MSG_STATUS));

  add_rect(1, Color_Bg_Black, col,  31, DWIN_WIDTH, DWIN_HEIGHT - 1);

  /* Hot end */
  add_icon(ICON, ICON_HotendTemp, col, 50);

  /* Heat bed */
  add_icon(ICON, ICON_BedTemp, col, 80);

  add_icon(ICON, ICON_Speed, col, 110);
  add_icon(ICON, ICON_FanSpeed, col, 140);

  add_icon(ICON, ICON_MaxSpeedX,  col + 10, 230);
  add_icon(ICON, ICON_MaxSpeedY,  col + 50, 230);
  add_icon(ICON, ICON_MaxSpeedZ,  col + 90, 230);

  update_lcd();
}

void Dwin_Lcd::draw_print_icon(void) {
  add_icon(ICON, ICON_Print_0, 10, 40);
  draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 45, 108, GET_TEXT_F(str_buffer, MSG_BUTTON_PRINT));
  // add_rect(0, Color_White, 10, 40, 119, 139);
}

void Dwin_Lcd::draw_main_frame() {
  uint16_t col = 360;
  /* Reset Frame */
  add_rect(1, Color_Bg_Green , 0, 0, col, 30);
  add_rect(1, Color_Bg_Black, 0, 31, col, DWIN_WIDTH);

  draw_string(false, false, DWIN_FONT_HEAD, Color_White, Color_Bg_Green, 14, 4, GET_TEXT_F(str_buffer, MSG_MAIN));

  add_icon(ICON, ICON_Print_0, 6, 40);
  draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 41, 108, GET_TEXT_F(str_buffer, MSG_BUTTON_PRINT));

  add_icon(ICON, ICON_Prepare_0, 126, 40);
  draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 156, 108, GET_TEXT_F(str_buffer, MSG_PREPARE));

  // add_icon(ICON, ICON_Leveling_0, 246, 40);
  // draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 276, 108, GET_TEXT_F(str_buffer, MSG_PREPARE));

  add_icon(ICON, ICON_Control_0, 6, 160);
  draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 36, 230, GET_TEXT_F(str_buffer, MSG_CONTROL));

  add_icon(ICON, ICON_Info_0, 126, 160);
  draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 138, 230, GET_TEXT_F(str_buffer, MSG_LEVEL_BED));

  // add_icon(ICON, ICON_Prepare_0, 246, 160);
  // draw_string(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 276, 230, GET_TEXT_F(str_buffer, MSG_PREPARE));

  update_lcd();
}