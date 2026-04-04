#pragma once
#include "AppScreenLayout.h"
#include "RadioManager.h"
#include "WebRadioPlayerScreen.h"

class RadioStationsScreen : public AppScreenLayout {
public:
 RadioStationsScreen() : AppScreenLayout("Radio Stations") {};

 void onCreate() override {
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_style_pad_gap(container, 10, 0);
   std::vector<RadioStation> &stations = RadioManager::getAll();
   if(stations.empty()) {
     lv_obj_t *lblNoStations = lv_label_create(container);
     lv_label_set_text(lblNoStations, "No stations found!");
     return;
   }

   for(auto& s : stations) {
     lv_obj_t *btnStation = lv_obj_create(container);
     lv_obj_set_size(btnStation, lv_pct(100), LV_SIZE_CONTENT);
     lv_obj_add_flag(btnStation, LV_OBJ_FLAG_CLICKABLE);
     lv_obj_t *lblStation = lv_label_create(btnStation);
     lv_label_set_text(lblStation, s.name.c_str());
      lv_obj_add_event_cb(btnStation, [](lv_event_t *e) {
          uint16_t id = (uint32_t)lv_event_get_user_data(e);
          ScreenManager::get().openModal(new WebRadioPlayerScreen(id));
      }, LV_EVENT_CLICKED, (void*)s.id);
   }
 }
};