#pragma once
#include "AppScreenLayout.h"
#include "RadioManager.h"
#include "AudioManager.h"

class WebRadioPlayerScreen : public AppScreenLayout {
private:
  uint16_t stationId;
public:
  WebRadioPlayerScreen(const uint16_t stationId = 1) : stationId(stationId), AppScreenLayout("Web Radio Player") {}

  void onCreate() override {
    if(WiFi.status() != WL_CONNECTED) {
      return;
    }
    AudioManager::init();

    lv_obj_t *noFound = lv_label_create(container);
    if(RadioManager::getById(stationId) == nullptr) {
      lv_label_set_text(noFound, "No radio found");

      return;
    } else {
      lv_label_set_text_fmt(noFound, "found %d radios", RadioManager::getAll().size());
    }

    RadioManager::play(stationId);
  }

  void onLoop() override {
    RadioManager::update();
  }

  void onDestroy() override {
    RadioManager::stop();
    AudioManager::deinit();
  }
};