#pragma once
#include "AppScreen.h"
#include "WiFiScreen.h"
#include <Arduino_GFX_Library.h>
#include "WebServerManager.h"
#include "WebServerScreen.h"
//#include "AudioManager.h"

class Arduino_GFX;
extern Arduino_GFX *gfx;
extern int brightness;

class ControlCenterScreen : public AppScreen {
private:
  lv_obj_t *btnWebServer;

  bool webServerRunning = WebServerManager::get().getStatus() == WebServerManager::Status::RUNNING;
public:
  void onCreate() override {
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(root, 5, 0);

    lv_obj_t *slidersContainer = lv_obj_create(root);
    lv_obj_remove_style_all(slidersContainer);
    //lv_obj_set_flex_grow(slidersContainer, 1);
    lv_obj_set_height(slidersContainer, lv_pct(100));
    lv_obj_set_flex_flow(slidersContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(slidersContainer, 0, 0);
    lv_obj_set_style_pad_row(slidersContainer, 0, 0);
    lv_obj_set_style_pad_column(slidersContainer, 0, 0);
    lv_obj_set_flex_align(slidersContainer, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /*
    lv_obj_t *sliderBright = lv_slider_create(slidersContainer);
    lv_obj_set_size(sliderBright, 20, lv_pct(100));
    lv_slider_set_range(sliderBright, 0, 255);
    lv_slider_set_value(sliderBright, 60, LV_ANIM_OFF);
    lv_obj_set_style_pad_all(sliderBright, 0, 0);
    lv_obj_set_style_pad_row(sliderBright, 0, 0);
    lv_obj_set_style_pad_column(sliderBright, 20, 0);

    lv_obj_t *sliderVol = lv_slider_create(slidersContainer);
    lv_obj_set_size(sliderVol, 20, lv_pct(100));
    lv_slider_set_range(sliderVol, 0, 255);
    lv_slider_set_value(sliderVol, 60, LV_ANIM_OFF);
    lv_obj_set_style_pad_all(sliderVol, 0, 0);
    lv_obj_set_style_pad_row(sliderVol, 0, 0);
    lv_obj_set_style_pad_column(sliderVol, 20, 0);
    */

    lv_obj_t *sliderAudio = createSlider(slidersContainer, LV_SYMBOL_AUDIO);
    lv_slider_set_range(sliderAudio, 0, 100);
    lv_obj_add_event_cb(sliderAudio, [](lv_event_t *e) {
        lv_obj_t *slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        //AudioManager::setVolume(value);
        // Handle audio volume change
    }, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *sliderBright = createSlider(slidersContainer, LV_SYMBOL_EYE_OPEN);
    lv_slider_set_range(sliderBright, 10, 255);
    lv_slider_set_value(sliderBright, brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(sliderBright, [](lv_event_t *e) {
        lv_obj_t *slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        brightness = value;
        gfx->Display_Brightness(value);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *right = lv_obj_create(root);
    lv_obj_remove_style_all(right);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(right, 1);
    lv_obj_set_style_pad_all(right, 15, 0);
    lv_obj_set_height(right, lv_pct(100));

    lv_obj_t *buttonsContainer = lv_obj_create(right);
    lv_obj_remove_style_all(buttonsContainer);
    lv_obj_set_width(right, LV_SIZE_CONTENT);
    //lv_obj_set_flex_flow(buttonsContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_flow(buttonsContainer, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(buttonsContainer, 0, 0);
    lv_obj_set_style_pad_row(buttonsContainer, 0, 0);
    lv_obj_set_style_pad_column(buttonsContainer, 0, 0);
    //lv_obj_set_scroll_dir(buttonsContainer, LV_DIR_VER);

    lv_obj_t *wifiButton = createIconButton(buttonsContainer, LV_SYMBOL_WIFI);
    lv_obj_add_event_cb(wifiButton, [](lv_event_t *e) {
      ScreenManager::get().openModal(new WiFiScreen());
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bleButton = createIconButton(buttonsContainer, LV_SYMBOL_BLUETOOTH);
    lv_obj_t *settingsButton = createIconButton(buttonsContainer, LV_SYMBOL_SETTINGS);
    lv_obj_t *audioButton = createIconButton(buttonsContainer, LV_SYMBOL_AUDIO);

    lv_obj_t *btnRestart = lv_btn_create(buttonsContainer);
    lv_obj_set_width(btnRestart, lv_pct(100));
    lv_obj_t *lblRestart = lv_label_create(btnRestart);
    lv_label_set_text(lblRestart, "Restart");
    lv_obj_add_event_cb(btnRestart, [](lv_event_t *e) {
      esp_restart();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *notifications = lv_obj_create(right);
    lv_obj_remove_style_all(notifications);
    //lv_obj_set_flex_grow(notifications, 1);
    lv_obj_set_flex_flow(notifications, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(notifications, lv_pct(100), LV_SIZE_CONTENT);

    lv_obj_t *bootTimeLabel = lv_label_create(notifications);

    uint64_t s = esp_timer_get_time() / 1000000ULL;

    uint32_t hours = s / 3600;
    s %= 3600;

    uint32_t minutes = s / 60;
    uint32_t seconds = s % 60;

    char buf[32];
    sprintf(buf, "%02u:%02u:%02u", hours, minutes, seconds);
    lv_label_set_text(bootTimeLabel, buf);

    btnWebServer = lv_obj_create(notifications);
    lv_obj_set_size(btnWebServer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_flag(btnWebServer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *lblWebServer = lv_label_create(btnWebServer);
    lv_label_set_text(lblWebServer, "WebServer Running..");
    lv_obj_add_event_cb(btnWebServer, [](lv_event_t *e) {
      ScreenManager::get().changeScreen(new WebServerScreen());
    }, LV_EVENT_CLICKED, NULL);

    if(!webServerRunning) {
      lv_obj_add_flag(btnWebServer, LV_OBJ_FLAG_HIDDEN);
    }
  }

  lv_obj_t *createSlider(lv_obj_t *parent, const char *symbol) {
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_height(container, lv_pct(100));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_width(container, lv_pct(50));
    lv_obj_set_style_pad_all(container, 20, 0);
    lv_obj_set_style_pad_row(container, 20, 0);
    lv_obj_set_style_pad_column(container, 20, 0);

    lv_obj_t *slider = lv_slider_create(container);
    //lv_obj_set_size(slider, 20, lv_pct(100));
    lv_obj_set_width(slider, 20);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, 0, 255);
    lv_slider_set_value(slider, 60, LV_ANIM_OFF);
    lv_obj_set_style_pad_all(slider, 0, 0);
    lv_obj_set_style_pad_row(slider, 0, 0);
    lv_obj_set_style_pad_column(slider, 20, 0);

    lv_obj_t *label = lv_label_create(container);
    lv_label_set_text(label, symbol);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    return slider;
  }
  
  lv_obj_t *createIconButton(lv_obj_t *parent, const char *symbol) {
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(container, lv_pct(50), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_pad_row(container, 0, 0);
    lv_obj_set_style_pad_column(container, 0, 0);
    lv_obj_t *btn = lv_btn_create(container);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, 60, 60);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    return btn;
  }

  void onLoop() override {
    bool wsRunning = WebServerManager::get().getStatus() == WebServerManager::Status::RUNNING;
    if(webServerRunning != wsRunning) {
      webServerRunning = wsRunning;
      if(webServerRunning) {
        lv_obj_clear_flag(btnWebServer, LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_obj_add_flag(btnWebServer, LV_OBJ_FLAG_HIDDEN);
      }
    }
  }
};