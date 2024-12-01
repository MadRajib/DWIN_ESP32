#include "dwin.h"
#include "../../include/language_en.h"
#include <Esp.h>
#include "../core/data_setup.h"
#include <math.h>

#define PAUSE_HEAT

#define USE_STRING_HEADINGS
#define USE_STRING_TITLES

#define DWIN_FONT_MENU font8x16
#define DWIN_FONT_STAT font10x20
#define DWIN_FONT_HEAD font10x20

#define MENU_CHAR_LIMIT  24

#define STATUS_Y (LCD_ROT ? 260 : 360)

#define MIN_MAXFEEDSPEED      1
#define MIN_MAXACCELERATION   1
#define MIN_MAXJERK           0.1
#define MIN_STEP              1

#define FEEDRATE_E      (60)

// Minimum unit (0.1) : multiple (10)
#define UNITFDIGITS 1
#define MINUNITMULT pow(10, UNITFDIGITS)

#define ENCODER_WAIT_MS                  20
#define DWIN_VAR_UPDATE_INTERVAL         512
#define DWIN_SCROLL_UPDATE_INTERVAL      SEC_TO_MS(2)
#define DWIN_REMAIN_TIME_UPDATE_INTERVAL SEC_TO_MS(20)

uint16_t TROWS = (LCD_ROT ? 5 : 6), MROWS = TROWS - 1,        // Total rows, and other-than-Back
                   TITLE_HEIGHT = (LCD_ROT ? 0 : 30),                   // Title bar height
                   MLINE = (LCD_ROT ? 43 : 53),                          // Menu line height
                   LBLX = 60,                           // Menu item label X
                   MENU_CHR_W = (LCD_ROT ? 18 : 8), STAT_CHR_W = (LCD_ROT ? 20 : 10);

#define MBASE(L) (49 + MLINE * (L))

#if ENABLED(PAUSE_HEAT)
  TERN_(HAS_HOTEND, uint16_t resume_hotend_temp = 0);
  TERN_(HAS_HEATED_BED, uint16_t resume_bed_temp = 0);
#endif

#if HAS_ZOFFSET_ITEM
  float dwin_zoffset = 0, last_zoffset = 0;
#endif
#define BABY_Z_VAR TERN(HAS_BED_PROBE, probe.offset.z, dwin_zoffset)

extern Printer printer;

/* Value Init */
HMI_value_t HMI_ValueStruct;
HMI_Flag_t HMI_flag{0};

millis_t dwin_heat_time = 0;

uint8_t checkkey = 0;

char str_buffer[20];
char formatBuffer[20];

typedef struct {
  uint8_t now, last;
  void set(uint8_t v) { now = last = v; }
  void reset() { set(0); }
  bool changed() { bool c = (now != last); if (c) last = now; return c; }
  bool dec() { if (now) now--; return changed(); }
  bool inc(uint8_t v) { if (now < (v - 1)) now++; else now = (v - 1); return changed(); }
} select_t;

select_t select_page{0}, select_file{0}, select_print{0}, select_prepare{0}
         , select_control{0}, select_axis{0}, select_temp{0}, select_motion{0}, select_tune{0}
         , select_PLA{0}, select_ABS{0}
         , select_speed{0}
         , select_acc{0}
         , select_jerk{0}
         , select_step{0}
         ;

inline bool HMI_IsChinese() { return HMI_flag.language == DWIN_CHINESE; }

void HMI_Init() {
  // HMI_SDCardInit();

  for (uint16_t t = 0; t <= 100; t += 2) {
    DWIN_ICON_Show(ICON, ICON_Bar, 15, 260);
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 15 + t * 242 / 100, 260, 257, 280);
    DWIN_UpdateLCD();
    delay(20);
  }

  // HMI_SetLanguage();
}

void Draw_Status_Title(const char * const title) {
  if (LCD_ROT)
    DWIN_Draw_String(false, false, DWIN_FONT_HEAD, Color_White, Color_Bg_Orange, (DWIN_WIDTH / 2) + 20, 4, (char*)title);
}

void Draw_Title(const char * const title) {
  DWIN_Draw_String(false, false, DWIN_FONT_HEAD, Color_White, LCD_ROT ? Color_Bg_Green : Color_Bg_Blue, 14, 4, (char*)title);
}

// void Draw_Title_Flash(const char * const title) {
//   DWIN_Draw_String(false, false, DWIN_FONT_HEAD, Color_White, LCD_ROT ? Color_Bg_Green : Color_Bg_Blue, 14, 4, (const char*)title);
//   // Draw_Status_Title(GET_TEXT_F(MSG_INFO_STATS_MENU));
// }

void Clear_Title_Bar() {
  if (LCD_ROT) {
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, 0, DWIN_WIDTH - 1, 30);
    DWIN_Draw_Rectangle(1, Color_Bg_Orange, (DWIN_WIDTH / 2) + 10, 0, DWIN_WIDTH - 1, 30);
  }

  DWIN_Draw_Rectangle(1, (LCD_ROT) ? Color_Bg_Green : Color_Bg_Blue, 0, 0, LCD_ROT ? (DWIN_WIDTH / 2) : DWIN_WIDTH, 30);    
}

void Clear_Menu_Area() {
  DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, 31, LCD_ROT ? STATUS_Y : DWIN_WIDTH, LCD_ROT ? DWIN_WIDTH : STATUS_Y);
}

void Clear_Main_Window() {
  Clear_Title_Bar();
  Clear_Menu_Area();
}

void ICON_Print() {
  if (select_page.now == 0) {
    if (LCD_ROT){
      DWIN_ICON_Show(ICON, ICON_Print_1, 10, 40);
      DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 45, 108, GET_TEXT_F(str_buffer, MSG_BUTTON_PRINT));
      DWIN_Draw_Rectangle(0, Color_White, 10, 40, 119, 139);
    } else {
      DWIN_ICON_Show(ICON, ICON_Print_1, 17, 130);
      DWIN_Draw_Rectangle(0, Color_White, 17, 130, 126, 229);
      if (HMI_IsChinese())
        DWIN_Frame_AreaCopy(1, 1, 447, 28, 460, 58, 201);
      else
        DWIN_Frame_AreaCopy(1, 1, 451, 31, 463, 57, 201);
    }  
  }
  else {
    if (LCD_ROT) {
      DWIN_ICON_Show(ICON, ICON_Print_0, 10, 40);
      DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 45, 108, GET_TEXT_F(str_buffer, MSG_BUTTON_PRINT));
    } else {
      DWIN_ICON_Show(ICON, ICON_Print_0, 17, 130);
      if (HMI_IsChinese())
        DWIN_Frame_AreaCopy(1, 1, 405, 28, 420, 58, 201);
      else
        DWIN_Frame_AreaCopy(1, 1, 423, 31, 435, 57, 201);
    }

  }
}

void ICON_Prepare() {
  if (select_page.now == 1) {
    if (LCD_ROT){
      DWIN_ICON_Show(ICON, ICON_Prepare_1, 130, 40);
      DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 160, 108, GET_TEXT_F(str_buffer, MSG_PREPARE));
      DWIN_Draw_Rectangle(0, Color_White, 130, 40, 239, 139);
    } else {
      DWIN_ICON_Show(ICON, ICON_Prepare_1, 145, 130);
      DWIN_Draw_Rectangle(0, Color_White, 145, 130, 254, 229);
      if (HMI_IsChinese())
        DWIN_Frame_AreaCopy(1, 31, 447, 58, 460, 186, 201);
      else
        DWIN_Frame_AreaCopy(1, 33, 451, 82, 466, 175, 201);
    }
  }
  else {
    if (LCD_ROT) {
      DWIN_ICON_Show(ICON, ICON_Prepare_0, 130, 40);
      DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 160, 108, GET_TEXT_F(str_buffer, MSG_PREPARE));
    } else {
      DWIN_ICON_Show(ICON, ICON_Prepare_0,145, 130);
      if (HMI_IsChinese())
        DWIN_Frame_AreaCopy(1, 31, 405, 58, 420, 186, 201);
      else
        DWIN_Frame_AreaCopy(1, 33, 423, 82, 438, 175, 201);
    }
  }
}

void ICON_Control() {
  if (select_page.now == 2) {
    if (LCD_ROT) {
      DWIN_ICON_Show(ICON, ICON_Control_1, 10, 160);
      DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 40, 230, GET_TEXT_F(str_buffer, MSG_CONTROL));
      DWIN_Draw_Rectangle(0, Color_White, 10, 160, 119, 259);
    } else {
      DWIN_ICON_Show(ICON, ICON_Control_1, 17, 246);
      DWIN_Draw_Rectangle(0, Color_White, 17, 246, 126, 345);
      if (HMI_IsChinese())
        DWIN_Frame_AreaCopy(1, 61, 447, 88, 460, 58, 318);
      else
        DWIN_Frame_AreaCopy(1, 85, 451, 132, 463, 48, 318);
    }
  }
  else {
    if (LCD_ROT) {
      DWIN_ICON_Show(ICON, ICON_Control_0, 10, 160);
      DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 40, 230, GET_TEXT_F(str_buffer, MSG_CONTROL));
    } else {
      DWIN_ICON_Show(ICON, ICON_Control_0, 17, 246);
      if (HMI_IsChinese())
        DWIN_Frame_AreaCopy(1, 61, 405, 88, 420, 58, 318);
      else
        DWIN_Frame_AreaCopy(1, 85, 423, 132, 434, 48, 318);
    }
  }
}

void ICON_StartInfo(bool show) {
  if (select_page.now == 3) {
    if(LCD_ROT){
      DWIN_ICON_Show(ICON, ICON_Info_1, 130, 160);
      DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 142, 230, GET_TEXT_F(str_buffer, MSG_INFO_SCREEN));
      DWIN_Draw_Rectangle(0, Color_White, 130, 160, 239, 259);
    } else {
      DWIN_ICON_Show(ICON, ICON_Info_1, 145, 246);
      DWIN_Draw_Rectangle(0, Color_White, 145, 246, 254, 345);
      if (HMI_IsChinese())
        DWIN_Frame_AreaCopy(1, 91, 447, 118, 460, 186, 318);
      else
        DWIN_Frame_AreaCopy(1, 132, 451, 159, 466, 186, 318);
    }
  }
  else {
    if(LCD_ROT) {
      DWIN_ICON_Show(ICON, ICON_Info_0, 130, 160);
      DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 142, 230, GET_TEXT_F(str_buffer, MSG_INFO_SCREEN));
    } else {
      DWIN_ICON_Show(ICON, ICON_Info_0, LCD_ROT ? 130 : 145, LCD_ROT ? 160: 246);
      if (HMI_IsChinese())
        DWIN_Frame_AreaCopy(1, 91, 405, 118, 420, 186, 318);
      else
        DWIN_Frame_AreaCopy(1, 132, 423, 159, 435, 186, 318);
    }
  }
}

// void ICON_Leveling(bool show) {
//   if (show) {
//     if (LCD_ROT) {
//       DWIN_ICON_Show(ICON, ICON_Info_1, 130, 160);
//       DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 152, 230, GET_TEXT_F(str_buffer, MSG_LEVEL_BED));
//       DWIN_Draw_Rectangle(0, Color_White, 130, 160, 239, 259);
//     } else {
//       DWIN_ICON_Show(ICON, ICON_Info_1, 145, 246);
//       DWIN_Draw_Rectangle(0, Color_White, 145, 246, 254, 345);
//       if (HMI_IsChinese())
//         DWIN_Frame_AreaCopy(1, 211, 447, 238, 460, 186, 318);
//       else
//         DWIN_Frame_AreaCopy(1, 84, 437, 120,  449, 182, 318);
//     }
//   }
//   else {
//     if (LCD_ROT) {
//       DWIN_ICON_Show(ICON, ICON_Info_0, 130, 160);
//       DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 152, 230, GET_TEXT_F(MSG_LEVEL_BED));
//     } else {
//       DWIN_ICON_Show(ICON, ICON_Info_0, 145, 246);
//       if (HMI_IsChinese())
//         DWIN_Frame_AreaCopy(1, 211, 405, 238, 420, 186, 318);
//       else
//         DWIN_Frame_AreaCopy(1, 84, 465, 120, 478, 182, 318);
//     }
//   }
// }

// void ICON_Tune() {
//   if (select_print.now == 0) {
//     DWIN_ICON_Show(ICON, ICON_Setup_1, LCD_ROT ? 2 : 8, LCD_ROT ? 180: 252);
//     DWIN_Draw_Rectangle(0, Color_White, LCD_ROT ? 2 : 8, LCD_ROT ? 180 : 252, LCD_ROT ? 81 : 87, LCD_ROT ? 271 : 351);
//     if (LCD_ROT) {
//       DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 23, 242, GET_TEXT_F(str_buffer, MSG_TUNE));
//     } else {
//       if (HMI_IsChinese())
//         DWIN_Frame_AreaCopy(1, 121, 447, 148, 458, 34, 325);
//       else
//         DWIN_Frame_AreaCopy(1,   0, 466,  34, 476, 31, 325);
//     }

//   }
//   else {
//     DWIN_ICON_Show(ICON, ICON_Setup_0, LCD_ROT ? 2 : 8, LCD_ROT ? 180: 252);
//     if (LCD_ROT){
//       DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 23, 242, GET_TEXT_F(str_buffer, MSG_TUNE));
//     } else {
//       if (HMI_IsChinese())
//         DWIN_Frame_AreaCopy(1, 121, 405, 148, 420, 34, 325);
//       else
//         DWIN_Frame_AreaCopy(1,   0, 438,  32, 448, 31, 325);
//     }
//   }
// }

// void ICON_Pause() {
//   if (select_print.now == 1) {
//     DWIN_ICON_Show(ICON, ICON_Pause_1, LCD_ROT ? 83 : 96, LCD_ROT ? 180 : 252);
//     DWIN_Draw_Rectangle(0, Color_White, LCD_ROT ? 83 : 96, LCD_ROT ? 180 : 252, LCD_ROT ? 162 : 175, LCD_ROT ? 271 : 351);
//     if (LCD_ROT) {
//       DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 104, 242, GET_TEXT_F(str_buffer, str_buffer, MSG_BUTTON_PRINT));
//     } else {
//       if (HMI_IsChinese())
//         DWIN_Frame_AreaCopy(1, 181, 447, 208, 459, 124, 325);
//       else
//         DWIN_Frame_AreaCopy(1, 177, 451, 216, 462, 116, 325);
//     }
//   }
//   else {
//     DWIN_ICON_Show(ICON, ICON_Pause_0, LCD_ROT ? 83 : 96, LCD_ROT  ? 180 : 252);
//     if (LCD_ROT) {
//       DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 104, 242, GET_TEXT_F(str_buffer, str_buffer, MSG_BUTTON_PRINT));
//     } else {
//       if (HMI_IsChinese())
//         DWIN_Frame_AreaCopy(1, 181, 405, 208, 420, 124, 325);
//       else
//         DWIN_Frame_AreaCopy(1, 177, 423, 215, 433, 116, 325);
//     }
//   }
// }

// void ICON_Continue() {
//   if (select_print.now == 1) {
//     DWIN_ICON_Show(ICON, ICON_Continue_1, LCD_ROT ? 83 : 96, LCD_ROT ? 180 : 252);
//     DWIN_Draw_Rectangle(0, Color_White, LCD_ROT ? 83 : 96, LCD_ROT ? 180 : 252, LCD_ROT ? 162 : 175, LCD_ROT ? 271 : 351);
//     if (LCD_ROT) {
//       DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 104, 242, F("Resume"));
//     } else {
//       if (HMI_IsChinese())
//         DWIN_Frame_AreaCopy(1, 1, 447, 28, 460, 124, 325);
//       else
//         DWIN_Frame_AreaCopy(1, 1, 452, 32, 464, 121, 325);
//     }
//   }
//   else {
//     DWIN_ICON_Show(ICON, ICON_Continue_0, LCD_ROT ? 83 : 96, LCD_ROT ? 180 : 252);
//     if (LCD_ROT) {
//       DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 104, 242, F("Resume"));
//     } else {
//       if (HMI_IsChinese())
//         DWIN_Frame_AreaCopy(1, 1, 405, 28, 420, 124, 325);
//       else
//         DWIN_Frame_AreaCopy(1, 1, 424, 31, 434, 121, 325);
//     }
//   }
// }

// void ICON_Stop() {
//   if (select_print.now == 2) {
//     DWIN_ICON_Show(ICON, ICON_Stop_1, LCD_ROT ? 164 : 184, LCD_ROT ? 180 : 252);
//     DWIN_Draw_Rectangle(0, Color_White, LCD_ROT ? 164 :184, LCD_ROT ? 180 : 252, LCD_ROT ? 243 : 263, LCD_ROT ? 271 : 351);
//     if (LCD_ROT) {
//       DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 188, 242, GET_TEXT_F(MSG_BUTTON_STOP));
//     } else {
//       if (HMI_IsChinese())
//         DWIN_Frame_AreaCopy(1, 151, 447, 178, 459, 210, 325);
//       else
//         DWIN_Frame_AreaCopy(1, 218, 452, 249, 466, 209, 325);
//     }
//   }
//   else {
//     DWIN_ICON_Show(ICON, ICON_Stop_0, LCD_ROT ? 164 : 184, LCD_ROT ? 180 : 252);
//     if (LCD_ROT) {
//       DWIN_Draw_String(false, false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 188, 242, GET_TEXT_F(MSG_BUTTON_STOP));
//     } else {
//     if (HMI_IsChinese())
//       DWIN_Frame_AreaCopy(1, 151, 405, 178, 420, 210, 325);
//     else
//       DWIN_Frame_AreaCopy(1, 218, 423, 247, 436, 209, 325);
//     }
//   }
// }


/*-----------------status area-----------------------*/

void DWIN_Draw_Signed_Float(uint8_t size, uint16_t bColor, uint8_t iNum, uint8_t fNum, uint16_t x, uint16_t y, long value) {
  if (value < 0) {
    DWIN_Draw_String(false, true, size, Color_White, bColor, x - 6, y, F("-"));
    DWIN_Draw_FloatValue(true, true, 0, size, Color_White, bColor, iNum, fNum, x, y, -value);
  }
  else {
    DWIN_Draw_String(false, true, size, Color_White, bColor, x - 6, y, F(" "));
    DWIN_Draw_FloatValue(true, true, 0, size, Color_White, bColor, iNum, fNum, x, y, value);
  }
}

// The status area is always on-screen, except during
// full-screen modal dialogs. (TODO: Keep alive during dialogs)
//

void Draw_Status_Area_text(void)
{
  #if HAS_HOTEND
    DWIN_Draw_String(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, LCD_ROT ? 280 : 28,
    LCD_ROT ? 50 : 384, SPRINTF_FROM_PROGMEM(str_buffer, formatBuffer, RANGE_FORMAT, (int)round(printer.extruder_temp), (int)round(printer.extruder_target_temp)));
  #endif

  #if HAS_HEATED_BED
    DWIN_Draw_String(false, true, DWIN_FONT_STAT, Color_White, Color_Bg_Black, LCD_ROT ? 400 : 28,
    LCD_ROT ? 50 : 384, SPRINTF_FROM_PROGMEM(str_buffer, formatBuffer, RANGE_FORMAT, (int)round(printer.bed_temp), (int)round(printer.bed_target_temp)));
  #endif

  DWIN_UpdateLCD();
  delay(5);
}

void Draw_Status_Area(const bool with_update) {

  DWIN_Draw_Rectangle(1, Color_Bg_Black, (DWIN_WIDTH / 2) + 10 , LCD_ROT ? 31 : STATUS_Y, DWIN_WIDTH, DWIN_HEIGHT - 1);

  #if HAS_HOTEND
    DWIN_ICON_Show(ICON, ICON_HotendTemp, LCD_ROT ? 250 : 10, LCD_ROT ? 50 : 383);
    DWIN_Draw_String(false, false, DWIN_FONT_STAT, Color_White, Color_Bg_Black, LCD_ROT ? 280 : 28,
    LCD_ROT ? 50 : 384, SPRINTF_FROM_PROGMEM(str_buffer, formatBuffer, RANGE_FORMAT, (int)round(printer.extruder_temp), (int)round(printer.extruder_target_temp)));
  #endif

  #if HAS_HEATED_BED
    DWIN_ICON_Show(ICON, ICON_BedTemp, LCD_ROT ? 370 : 10, LCD_ROT ? 50 : 416);
    DWIN_Draw_String(false, false, DWIN_FONT_STAT, Color_White, Color_Bg_Black, LCD_ROT ? 400 : 28,
    LCD_ROT ? 50 : 384, SPRINTF_FROM_PROGMEM(str_buffer, formatBuffer, RANGE_FORMAT, (int)round(printer.bed_temp), (int)round(printer.bed_target_temp)));
  #endif

  DWIN_ICON_Show(ICON, ICON_MaxSpeedX,  LCD_ROT ? 250 : 10, LCD_ROT ? 80 : 456);
  DWIN_ICON_Show(ICON, ICON_MaxSpeedY,  LCD_ROT ? 325 : 95, LCD_ROT ? 80 : 456);
  DWIN_ICON_Show(ICON, ICON_MaxSpeedZ,  LCD_ROT ? 395 : 180, LCD_ROT ? 80 : 456);

  DWIN_ICON_Show(ICON, ICON_Speed, LCD_ROT ? 250 : 113, LCD_ROT ? 110 : 383);
  #if HAS_FAN
    DWIN_ICON_Show(ICON, ICON_FanSpeed, LCD_ROT ? 325 : 187, LCD_ROT ? 110 : 383);
  #endif

  #if HAS_ZOFFSET_ITEM
    DWIN_ICON_Show(ICON, ICON_Zoffset, LCD_ROT ? 395 : 187, LCD_ROT ? 110 : 416);
  #endif

  Draw_Status_Area_text();
}
/*---------------------end---------------------------*/

void Goto_MainMenu() {
  checkkey = MainMenu;

  Clear_Main_Window();

  if (HMI_IsChinese()) {
    DWIN_Frame_AreaCopy(1, 2, 2, 27, 14, 14, 9); // "Home"
  } else {
    Draw_Title(GET_TEXT_F(str_buffer, MSG_MAIN));
    Draw_Status_Title(GET_TEXT_F(str_buffer, MSG_STATUS));
  }

  // Not enough space for Crealtiy's logo in landscape layout
  if (!LCD_ROT) DWIN_ICON_Show(ICON, ICON_LOGO, 71, 52);

  ICON_Print();
  ICON_Prepare();
  ICON_Control();
  ICON_StartInfo(true);
  
  // TERN(HAS_ONESTEP_LEVELING, ICON_Leveling, ICON_StartInfo)(select_page.now == 3);
}

void HMI_StartFrame(const bool with_update) {
  Goto_MainMenu();
  Draw_Status_Area(with_update);
}