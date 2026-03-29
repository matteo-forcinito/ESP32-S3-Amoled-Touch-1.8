#pragma once
#include "AppScreen.h"
#include "WiFiScreen.h"
#include "WifiManager.h"
class WifiQualityScreen : public AppScreen {
private:
  bool ready = false;
  char last = ' ';
  unsigned int updateTime = 0;

  WifiNetwork connected;

  lv_obj_t *wifiRssi;
public:
    void onCreate() override {
      lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
lv_obj_set_flex_align(root,
                      LV_FLEX_ALIGN_CENTER,  // orizzontalmente
                      LV_FLEX_ALIGN_CENTER,  // verticalmente tra i figli
                      LV_FLEX_ALIGN_CENTER); // spazio tra items
      lv_obj_set_style_pad_all(root, 10, 0);

      if(WiFi.status() != WL_CONNECTED) {
        lv_obj_t *lbl = lv_label_create(root);
        lv_label_set_text(lbl, "Devi essere connesso al Wifi!");
        lv_obj_t *btn = lv_btn_create(root);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
          ScreenManager::get().openModal(new WiFiScreen());
        }, LV_EVENT_CLICKED, NULL);
        lbl = lv_label_create(btn);
        lv_label_set_text(lbl, "Connect");

        return;
      } 
      connected = WifiManager::getConnected();
      ready = connected.ssid.length() > 0;

      if(!ready) return;

      lv_obj_t *textContainer = lv_obj_create(root);lv_obj_set_size(textContainer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

// Centra i figli dentro textContainer verticalmente e orizzontalmente
lv_obj_set_flex_flow(textContainer, LV_FLEX_FLOW_COLUMN);
lv_obj_set_flex_align(textContainer,
                      LV_FLEX_ALIGN_CENTER,  // orizzontalmente
                      LV_FLEX_ALIGN_CENTER,  // verticalmente tra i figli
                      LV_FLEX_ALIGN_CENTER); // spazio tra items
      lv_obj_t *wifiName = lv_label_create(textContainer);
      lv_label_set_text(wifiName, connected.ssid.c_str());
      wifiRssi = lv_label_create(textContainer);
      lv_label_set_text(wifiRssi, String(WiFi.RSSI()).c_str());
      updateTime = millis();
    }

    void onLoop() override {
      if(!ready || millis() - updateTime < 500) return;
      updateTime = millis();

      char current = ' ';
      int rssi = WiFi.RSSI();

      lv_label_set_text(wifiRssi, String(rssi).c_str());

      if(rssi > -60) {
        current = 'v';
      } else if(rssi > -80) {
        current = 'g';
      } else {
        current = 'r';
      }

      if(current == last) return;
      lv_color_t bgColor;
      switch(current) {
        case 'v': {
          bgColor = lv_color_hex(0x00FF00); // verde
          break;
        }

        case 'g': {
          bgColor = lv_color_hex(0xFFFF00); // giallo
          break;
        }

        case 'r': {
          bgColor = lv_color_hex(0xFF0000); // giallo
        }
      }
      lv_obj_set_style_bg_color(root, bgColor, 0);
      lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    }
};