#pragma once
#include "SimpleScreen.h"
#include "AlarmManager.h"
#include "AudioManager.h"

class AlarmTriggerScreen : public SimpleScreen {
private:
  uint32_t id;
  Alarm *alarm = nullptr;
  unsigned long updateTime = 0;
public:
  AlarmTriggerScreen(uint32_t id) : id(id) {}

  void onCreate() override {
    gfx->fillScreen(BLACK);

    alarm = AlarmManager::getById(id);
    if(alarm == nullptr) {
      gfx->setCursor(50, 50);
      gfx->setTextSize(3);
      gfx->print("Error, no alarm");

      return;
    } 

    gfx->setCursor(50, 50);
    gfx->setTextSize(4);
    gfx->print(alarm->title.c_str());

    AudioManager::init();
    updateTime = millis();
  }

  void onGesture(Gesture g) override {
    if(g == Gesture::LEFT || g == Gesture::RIGHT) {
      AudioManager::stopAlarm();
    }
  }

  void onLoop() override {
    if(updateTime != 0 && millis() - updateTime > 500 && alarm != nullptr) {
      updateTime = 0;
      AudioManager::startAlarm();
    }
  }

  void onDestroy() override {
    AudioManager::stopAlarm();
    AudioManager::deinit();
  }
};