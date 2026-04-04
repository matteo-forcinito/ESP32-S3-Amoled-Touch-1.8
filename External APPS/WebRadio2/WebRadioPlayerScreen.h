#pragma once
#include "AppScreenLayout.h"
#include "RadioManager.h"
#include "AudioManager.h"
#include "RadioStation.h"

class WebRadioPlayerScreen : public AppScreenLayout {
private:
  uint16_t stationId;
  lv_obj_t *noFound = nullptr;
  lv_obj_t *status = nullptr;
  lv_obj_t *btnClose = nullptr;

  bool wasPlaying = true;
  bool ready = false;
  RadioManager::State lastState = RadioManager::State::INIT;
public:
  WebRadioPlayerScreen(const uint16_t stationId = 1) : stationId(stationId), AppScreenLayout("Web Radio Player") {}

  void onShow() override {
    RadioStation station;
    if (RadioManager::getById(stationId, station)) {
      lv_label_set_text(titleLabel, station.name.c_str());
    }
  }

  void onCreate() override {
    //RadioManager::s();
    if(WiFi.status() != WL_CONNECTED) {
      return;
    }

    noFound = lv_label_create(container);
    RadioStation station;
    if (RadioManager::getById(stationId, station))  {
      //lv_label_set_text(titleLabel, station.name.c_str());
      lv_label_set_text_fmt(noFound, "found %d radios", RadioManager::getAll().size());
      
      lv_obj_t *sliderContainer = lv_obj_create(container);
      lv_obj_remove_style_all(sliderContainer);
      lv_obj_set_flex_flow(sliderContainer, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(sliderContainer, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_set_size(sliderContainer, lv_pct(100), LV_SIZE_CONTENT);
      lv_obj_t *slider = lv_slider_create(sliderContainer);
      lv_obj_set_height(slider, 30);
      lv_obj_set_flex_grow(slider, 1);
      lv_slider_set_range(slider, 0, 100);
      lv_slider_set_value(slider, AudioManager::getVolume(), LV_ANIM_OFF);
      lv_obj_add_event_cb(slider, [](lv_event_t *e) {
          lv_obj_t *slider = lv_event_get_target(e);
          int value = lv_slider_get_value(slider);
          AudioManager::setVolume(value);
          // Handle audio volume change
      }, LV_EVENT_VALUE_CHANGED, NULL);

      lv_obj_t *label = lv_label_create(sliderContainer);
      lv_label_set_text(label, LV_SYMBOL_AUDIO);
      lv_obj_center(label);
      lv_obj_set_style_text_color(label, lv_color_white(), 0);
      lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

      btnClose = lv_btn_create(container);
      lv_obj_t *lblClose = lv_label_create(btnClose);
      lv_label_set_text(lblClose, "Close");
      lv_obj_add_event_cb(btnClose, [](lv_event_t *e) {
        RadioManager::stop();
      }, LV_EVENT_CLICKED, NULL);

      status = lv_label_create(container);
      
      lv_label_set_text(status, "Connecting...");
      wasPlaying = RadioManager::play(stationId) && WiFi.status() == WL_CONNECTED;
      if(wasPlaying) ready = true;
    }
  }

  void onLoop() override {
    if(!ready) return;

    RadioManager::State state = RadioManager::getState();
    if(state == lastState) return;

    switch(state) {
      case RadioManager::State::CHANGING: {
        lv_label_set_text(status, "Changing Station...");
        break;
      }
      case RadioManager::State::CONNECTING: {
        lv_label_set_text(status, "Connecting to station...");
        break;
      }
      case RadioManager::State::STOPPED: {
        lv_label_set_text(status, "STOPPED!");
        break;
      }
      case RadioManager::State::PLAYING: {
        lv_label_set_text(status, "Playing..");
        break;
      }
    }
    lastState = state;
  }

  void onDestroy() override {
    //RadioManager::stop();
  }
};