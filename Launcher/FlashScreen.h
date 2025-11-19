#pragma once
#include "AppScreen.h"
#include "Arduino_GFX_Library.h"
//#include "HomeScreen.h"
#include "HomeScreen.h"

extern Arduino_GFX *gfx;
extern int brightness;

class FlashScreen : public AppScreen {
protected:
  AppState id = APP_FLASH;
public:
  void onCreate() override {
    lv_obj_set_width(root, lv_pct(100));
    lv_obj_set_height(root, lv_pct(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0xFFFFFF), 0);
    gfx->Display_Brightness(255);
  }

  void onDestroy() override {
    gfx->Display_Brightness(brightness);
  }
};
