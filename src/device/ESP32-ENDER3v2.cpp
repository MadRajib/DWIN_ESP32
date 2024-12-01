#include "../conf/global_config.h"
#include "lvgl.h"
#include "../core/lv_setup.h"
#include "screen_driver.h"

#define CPU_FREQ_HIGH 240
#define CPU_FREQ_LOW 80
#define TOUCH_THRESHOLD 600

void screen_setBrightness(unsigned char brightness)
{
    // // calculate duty, 4095 from 2 ^ 12 - 1
    // uint32_t duty = (4095 / 255) * brightness;

    // // write duty to LEDC
    // ledcWrite(0, duty);
}

void screen_lv_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    // uint32_t w = (area->x2 - area->x1 + 1);
    // uint32_t h = (area->y2 - area->y1 + 1);

    // tft.startWrite();
    // tft.setAddrWindow(area->x1, area->y1, w, h);
    // tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    // tft.endWrite();

    // lv_disp_flush_ready(disp);
}

void screen_lv_touchRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    // if (tft.getTouch( &touchX, &touchY, TOUCH_THRESHOLD))
    // {
    //     data->state = LV_INDEV_STATE_PR;
    //     data->point.x = touchX;
    //     data->point.y = touchY;
    // }
    // else
    // {
    //     data->state = LV_INDEV_STATE_REL;
    // }
}

void set_invert_display(){
    // tft.invertDisplay(get_current_printer_config()->invert_colors);
}

void screen_setup()
{
    // lv_init();

    // tft.init();
    // tft.fillScreen(TFT_BLACK);
    // tft.invertDisplay(false);
    // delay(300);

    // tft.setRotation(1);
    // tft.setTouch( calData );

    // if (global_config.display_mode) {
    //     // <3 https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/cyd.md#the-display-doesnt-look-as-good
    //     tft.writecommand(ILI9341_GAMMASET); //Gamma curve selected
    //     tft.writedata(2);
    //     delay(120);
    //     tft.writecommand(ILI9341_GAMMASET); //Gamma curve selected
    //     tft.writedata(1);
    // }

    // ledcSetup(0, 5000, 12);
    // ledcAttachPin(21, 0);

    // pinMode(TFT_BL, OUTPUT);
    // digitalWrite(TFT_BL, HIGH);

    // lv_disp_draw_buf_init(&draw_buf, buf, NULL, CYD_SCREEN_HEIGHT_PX * CYD_SCREEN_WIDTH_PX / 10);

    // /*Initialize the display*/
    // static lv_disp_drv_t disp_drv;
    // lv_disp_drv_init(&disp_drv);
    // disp_drv.hor_res = CYD_SCREEN_WIDTH_PX;
    // disp_drv.ver_res = CYD_SCREEN_HEIGHT_PX;
    // disp_drv.flush_cb = screen_lv_flush;
    // disp_drv.draw_buf = &draw_buf;
    // lv_disp_drv_register(&disp_drv);

    // /*Initialize the (dummy) input device driver*/
    // static lv_indev_drv_t indev_drv;
    // lv_indev_drv_init(&indev_drv);
    // indev_drv.type = LV_INDEV_TYPE_POINTER;
    // indev_drv.read_cb = screen_lv_touchRead;
    // lv_indev_drv_register(&indev_drv);
}
