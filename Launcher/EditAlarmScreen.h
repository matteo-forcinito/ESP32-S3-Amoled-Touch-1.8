#pragma once
#include "AppScreenLayout.h"
#include "AlarmManager.h"

class EditAlarmScreen : public AppScreenLayout {
private:
  uint32_t id = 0;
  Alarm alarm;
  bool ready = false;

  enum class Status {
    IDLE,
    ERROR
  };

  lv_obj_t* hour = nullptr;
  lv_obj_t* min = nullptr;
  lv_obj_t *lblStatus = nullptr;
public:
  EditAlarmScreen(uint32_t id = 0) : AppScreenLayout("Edit Alarm"), id(id) {}

  void onCreate() override {
    Alarm* a = nullptr;
    if(id != 0) {
      a = AlarmManager::getById(id);
      alarm = *a;
      ready = true;
    }
    if(!ready) {
      lv_obj_t *l = lv_label_create(container);
      lv_label_set_text(l, "No alarm found");

      return;
    }

    lv_obj_t *title = lv_label_create(container);
    lv_label_set_text(title, alarm.title.c_str());
    lv_obj_t *descr = lv_label_create(container);
    lv_label_set_text(descr, alarm.description.c_str());

    lv_obj_t *timeContainer = lv_obj_create(container);
    lv_obj_remove_style_all(timeContainer);
    lv_obj_set_size(timeContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(timeContainer, LV_FLEX_FLOW_ROW);

    lv_obj_t *hourContainer = lv_obj_create(timeContainer);
    lv_obj_set_flex_grow(hourContainer, 1);
    lv_obj_set_flex_flow(hourContainer, LV_FLEX_FLOW_COLUMN);
    //lv_obj_set_width(hourContainer, lv_pct(100));

    lv_obj_t *btnIncHour = lv_btn_create(hourContainer);
    lv_obj_t *lblBtnAdd = lv_label_create(btnIncHour);
    lv_label_set_text(lblBtnAdd, "+");
    lv_obj_add_event_cb(btnIncHour, [](lv_event_t *e) {
        auto *screen = (EditAlarmScreen*) lv_event_get_user_data(e);

        screen->alarm.hour = (screen->alarm.hour + 1) % 24;

        lv_label_set_text(screen->hour, String(screen->alarm.hour).c_str());

    }, LV_EVENT_CLICKED, this);

    hour = lv_label_create(hourContainer);
    lv_label_set_text(hour, String(alarm.hour).c_str());

    lv_obj_t *btnDecHour = lv_btn_create(hourContainer);
    lv_obj_t *lblBtnDec = lv_label_create(btnDecHour);
    lv_label_set_text(lblBtnDec, "-");
    lv_obj_add_event_cb(btnDecHour, [](lv_event_t *e) {
        auto *screen = (EditAlarmScreen*) lv_event_get_user_data(e);

        screen->alarm.hour = (screen->alarm.hour + 23) % 24;

        lv_label_set_text(screen->hour, String(screen->alarm.hour).c_str());

    }, LV_EVENT_CLICKED, this);

    lv_obj_t *minContainer = lv_obj_create(timeContainer);
    lv_obj_set_flex_grow(minContainer, 1);
    lv_obj_set_flex_flow(minContainer, LV_FLEX_FLOW_COLUMN);
    //lv_obj_set_width(hourContainer, lv_pct(100));

    lv_obj_t *btnIncMin = lv_btn_create(minContainer);
    lv_obj_t *lblBtnIncMin = lv_label_create(btnIncMin);
    lv_label_set_text(lblBtnIncMin, "+");
    lv_obj_add_event_cb(btnIncMin, [](lv_event_t *e) {
        auto *screen = (EditAlarmScreen*) lv_event_get_user_data(e);

        screen->alarm.minute = (screen->alarm.minute + 1) % 60;

        lv_label_set_text(screen->min, String(screen->alarm.minute).c_str());

    }, LV_EVENT_CLICKED, this);

    min = lv_label_create(minContainer);
    lv_label_set_text(min, String(alarm.minute).c_str());

    lv_obj_t *btnDecMin = lv_btn_create(minContainer);
    lv_obj_t *lblBtnDecMin = lv_label_create(btnDecMin);
    lv_label_set_text(lblBtnDecMin, "-");
    lv_obj_add_event_cb(btnDecMin, [](lv_event_t *e) {
        auto *screen = (EditAlarmScreen*) lv_event_get_user_data(e);

        screen->alarm.minute = (screen->alarm.minute + 59) % 60;

        lv_label_set_text(screen->min, String(screen->alarm.minute).c_str());

    }, LV_EVENT_CLICKED, this);

    lblStatus = lv_label_create(container);

    lv_obj_t *btnSave = lv_btn_create(container);
    lv_obj_t *lblSave = lv_label_create(btnSave);
    lv_label_set_text(lblSave, "Save");
    lv_obj_add_event_cb(btnSave, [](lv_event_t *e) {
      auto *screen = (EditAlarmScreen*) lv_event_get_user_data(e);
      if(AlarmManager::edit(screen->alarm)) {
        screen->close();
      } else {
        lv_label_set_text_fmt(screen->lblStatus, "Error updating alarm with id: %s", String(screen->alarm.id).c_str());
      }
    }, LV_EVENT_CLICKED, this);
  }
};