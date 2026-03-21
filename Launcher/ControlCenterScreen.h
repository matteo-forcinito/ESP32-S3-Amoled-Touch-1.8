#pragma once
#include "AppScreen.h"
#include "AudioManager.h"

extern Arduino_GFX *gfx;
extern int brightness;

class ControlCenterScreen : public AppScreen {
public:
  void onCreate() override {
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);

    lv_obj_t *slidersContainer = lv_obj_create(root);
    lv_obj_remove_style_all(slidersContainer);
    lv_obj_set_flex_grow(slidersContainer, 1);
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
        AudioManager::setVolume(value);
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

    lv_obj_t *buttonsContainer = lv_obj_create(root);
    lv_obj_remove_style_all(buttonsContainer);
    lv_obj_set_height(buttonsContainer, lv_pct(100));
    lv_obj_set_flex_grow(buttonsContainer, 1);
    //lv_obj_set_flex_flow(buttonsContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_flow(buttonsContainer, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(buttonsContainer, 0, 0);
    lv_obj_set_style_pad_row(buttonsContainer, 0, 0);
    lv_obj_set_style_pad_column(buttonsContainer, 0, 0);
    //lv_obj_set_scroll_dir(buttonsContainer, LV_DIR_VER);

    lv_obj_t *wifiButton = createIconButton(buttonsContainer, LV_SYMBOL_WIFI);
    lv_obj_t *bleButton = createIconButton(buttonsContainer, LV_SYMBOL_BLUETOOTH);
    lv_obj_t *settingsButton = createIconButton(buttonsContainer, LV_SYMBOL_SETTINGS);
    lv_obj_t *audioButton = createIconButton(buttonsContainer, LV_SYMBOL_AUDIO);

    lv_obj_t *bootTimeLabel = lv_label_create(buttonsContainer);

    uint64_t s = esp_timer_get_time() / 1000000ULL;

    uint32_t hours = s / 3600;
    s %= 3600;

    uint32_t minutes = s / 60;
    uint32_t seconds = s % 60;

    char buf[32];
    sprintf(buf, "%02u:%02u:%02u", hours, minutes, seconds);

    
    lv_label_set_text(bootTimeLabel, buf);

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
    lv_obj_set_width(container, lv_pct(50));
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
};