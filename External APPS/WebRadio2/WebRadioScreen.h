#pragma once
#include "AppScreenLayout.h"
#include "RadioManager.h"
#include "AudioManager.h"
#include "WiFiScreen.h"
#include "RadioStationsScreen.h"

class WebRadioScreen : public AppScreenLayout {
private:
  AppState id = APP_WEB_RADIO;

  enum class Status {
    IDLE,
    NO_WIFI,
    LAST_STATUS // for init 
  };
  Status status = Status::IDLE;
  Status lastStatus = Status::LAST_STATUS;
public:
  WebRadioScreen() : AppScreenLayout("Web Radio") {}

  void onCreate() override {
    if(WiFi.status() != WL_CONNECTED) {
      status = Status::NO_WIFI;

      return;
    }
    RadioManager::start();
  }

  void onLoop() override {
    if(status == lastStatus) {
      if(status == Status::NO_WIFI && WiFi.status() == WL_CONNECTED) {
        status = Status::IDLE;
      } 
        return;
    }

    lv_obj_clean(container);

    switch(status) {
      case Status::IDLE: {
        lv_obj_t* btnOpenRadios = lv_btn_create(container);
        lv_obj_t* lblOpenRadios = lv_label_create(btnOpenRadios);
        lv_label_set_text(lblOpenRadios, "Radio Stations");
        lv_obj_add_event_cb(btnOpenRadios, [](lv_event_t *e) {
          ScreenManager::get().openModal(new RadioStationsScreen());
        }, LV_EVENT_CLICKED, NULL);

        break;
      }
      case Status::NO_WIFI: {
        lv_obj_t *lblNoWifi = lv_label_create(container);
        lv_label_set_text(lblNoWifi, "devi essere connesso al wifi!");
        lv_obj_t *btnNoWifi = lv_btn_create(container);
        lv_obj_t *lblBtnNoWifi = lv_label_create(btnNoWifi);
        lv_label_set_text(lblBtnNoWifi, "connect");
        lv_obj_add_event_cb(btnNoWifi, [](lv_event_t *e) {
          ScreenManager::get().openModal(new WiFiScreen());
        }, LV_EVENT_CLICKED, NULL);

        break;
      }
    }

    lastStatus = status; 
  }

  void onDestroy() override {
    USBSerial.print("stopping radio..");
    RadioManager::stop();
  }
};