#pragma once
#include "AppScreen.h"
#include "AlarmManager.h"
#include "AudioManager.h"

class AlarmTriggerScreen : public AppScreen {
private:
  uint32_t id;
  Alarm *alarm = nullptr;
  unsigned long updateTime = 0;
  unsigned long postponeTime = 0;
public:
  AlarmTriggerScreen(uint32_t id) : id(id) {}

  void onCreate() override {
    lv_obj_set_style_bg_color(root, lv_color_hex(0xFF0000), 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root,
                  LV_FLEX_ALIGN_CENTER,  // orizzontalmente
                  LV_FLEX_ALIGN_CENTER,  // verticalmente tra i figli
                  LV_FLEX_ALIGN_CENTER); // spazio tra items
    lv_obj_set_style_pad_all(root, 10, 0);

    alarm = AlarmManager::getById(id);
    if(alarm == nullptr) {
      lv_obj_t *c = lv_label_create(root);
      lv_label_set_text(c, "No Alarm Found!");

      return;
    }
    
    AudioManager::init();
    updateTime = millis();

    lv_obj_t *cont = lv_obj_create(root);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, alarm->title.c_str());
    lv_obj_t *descr = lv_label_create(cont);
    lv_label_set_text(descr, alarm->description.c_str());
    lv_obj_t *btn = lv_btn_create(cont);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "Turn OFF");
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      AlarmTriggerScreen *screen = (AlarmTriggerScreen*) lv_event_get_user_data(e);
      AudioManager::stopAlarm();
      screen->postponeTime = millis();
    }, LV_EVENT_CLICKED, this);
  }

  void onLoop() override {
    if(updateTime != 0 && millis() - updateTime > 500 && alarm != nullptr) {
      updateTime = 0;
      AudioManager::startAlarm();
    }
    if(postponeTime != 0 && millis() - postponeTime > (60000 * 2)) {
      postponeTime = 0;
      AudioManager::startAlarm();
    }
  }

  void onDestroy() override {
    AudioManager::stopAlarm();
    AudioManager::deinit();
  }
};