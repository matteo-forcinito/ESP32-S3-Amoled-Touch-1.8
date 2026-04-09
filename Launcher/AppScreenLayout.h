#pragma once
#include "AppScreen.h"
#include "ControlCenterScreen.h"

class AppScreenLayout : public AppScreen {
protected:
  lv_obj_t *container = nullptr;
private:
  String title; 

  bool isCharging = false;
  bool wifiConnected = false;
  unsigned long lastUpdate = 0;

  lv_obj_t *titleLabel = nullptr;
  lv_obj_t *timeLabel = nullptr;
  lv_obj_t *chargingLabel = nullptr;
  lv_obj_t *battery = nullptr;
  lv_obj_t *lblRam = nullptr;

  lv_obj_t *connections;
  lv_obj_t *wifiConnection;

  void update() {
    RTC_DateTime datetime = rtc.getDateTime();

    lastUpdate = millis();

    char timeString[6];
    sprintf(timeString, "%02d:%02d",
            datetime.hour,
            datetime.minute);

    lv_label_set_text(timeLabel, timeString);

    int batteryPercent = power.getBatteryPercent();
    lv_label_set_text_fmt(battery, "%d%%", batteryPercent);
    lv_label_set_text(lblRam, getRamUsage().c_str());
    lastUpdate = millis();
  }

  String getRamUsage() {
      size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
      size_t totalHeap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
      size_t usedHeap = totalHeap - freeHeap;

      char buf[64];
      sprintf(buf, "%dKB / %dKB (%d%%)",
          usedHeap / 1024,
          totalHeap / 1024,
          (usedHeap * 100) / totalHeap
      );

      return String(buf);
  }
public:
  AppScreenLayout(const String& title = String("No Title")) : title(title) {}

  void loop() override {
    if(!modal) {
      if(power.isCharging() != isCharging) {
        isCharging = power.isCharging();
        if(isCharging) {
          lv_obj_clear_flag(chargingLabel, LV_OBJ_FLAG_HIDDEN);
        } else {
          lv_obj_add_flag(chargingLabel, LV_OBJ_FLAG_HIDDEN);
        }
      }
      if(lastUpdate != 0 && (millis() - lastUpdate > 1000)) {
        update();
      }
      if(WiFi.getMode() != WIFI_MODE_NULL && !wifiConnected) {
          lv_obj_clear_flag(wifiConnection, LV_OBJ_FLAG_HIDDEN);
          wifiConnected = true;
      }
      if(WiFi.getMode() == WIFI_MODE_NULL && wifiConnected) {
          lv_obj_add_flag(wifiConnection, LV_OBJ_FLAG_HIDDEN);
          wifiConnected = false;
      }
    }
    AppScreen::loop();
  }

  // Crea la schermata e richiama onCreate()
  void create() override {
      AppScreen::create();
      lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_style_pad_all(root, 10, 0);
      lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);

      lv_obj_t *header = lv_obj_create(root);
      lv_obj_remove_style_all(header);
      lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
      lv_obj_set_size(header, 340, LV_SIZE_CONTENT);  // larghezza uguale allo schermo
      lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

      // Imposto flex row per contenitore
      lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(header,
                            LV_FLEX_ALIGN_SPACE_BETWEEN,  // title a destra, battery a sinistra
                            LV_FLEX_ALIGN_CENTER,          // allinea verticalmente
                            LV_FLEX_ALIGN_CENTER);         // cross axis
      lv_obj_set_style_pad_all(header, 10, 0); // padding interno

      lv_obj_add_flag(header, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(header, [](lv_event_t *e) {
        ScreenManager::get().openModal(new ControlCenterScreen());
      }, LV_EVENT_CLICKED, NULL);
  /*
      lv_obj_add_event_cb(header, [](lv_event_t *e) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_event_get_indev(e));
        //if (dir == LV_DIR_BOTTOM) {
            ScreenManager::get().changeScreen(new ControlCenterScreen());
            // Azione da eseguire
        //}
      }, LV_EVENT_CLICKED, NULL);
  */
      timeLabel = lv_label_create(header);

      connections = lv_obj_create(header);
      lv_obj_remove_style_all(connections);
      lv_obj_set_flex_flow(connections, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_grow(connections, 1);
      lv_obj_set_height(connections, LV_SIZE_CONTENT);
      lv_obj_set_flex_align(
          connections,
          LV_FLEX_ALIGN_CENTER,   // orizzontale
          LV_FLEX_ALIGN_CENTER,  // verticale
          LV_FLEX_ALIGN_CENTER   // allineamento contenuto
      ); 

      lblRam = lv_label_create(connections);
      lv_label_set_text(lblRam, getRamUsage().c_str());

      lv_obj_t *headerRight = lv_obj_create(header);
      lv_obj_remove_style_all(headerRight);
      lv_obj_set_flex_flow(headerRight, LV_FLEX_FLOW_ROW);
      lv_obj_set_size(headerRight, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

      chargingLabel = lv_label_create(headerRight);
      lv_label_set_text(chargingLabel, LV_SYMBOL_CHARGE);
      lv_obj_set_style_text_color(chargingLabel, lv_color_hex(0xFFFF00), 0);

      if(!power.isCharging()) {
          lv_obj_add_flag(chargingLabel, LV_OBJ_FLAG_HIDDEN);
      }
      // Label batteria
      battery = lv_label_create(header);
      lv_obj_align(timeLabel, LV_ALIGN_TOP_MID, 0, 10);
      
      update();

      // ---- TITOLO ----
      titleLabel = lv_label_create(root);
      lv_obj_set_width(titleLabel, lv_pct(100));
      lv_label_set_text(titleLabel, title.c_str());
      lv_obj_set_style_text_color(titleLabel, lv_color_white(), 0);
      lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_24, 0);
      lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);
      lv_obj_set_flex_align(titleLabel,
                            LV_FLEX_ALIGN_START,  // allinea gli elementi all'inizio in orizzontale
                            LV_FLEX_ALIGN_START,  // allinea all'inizio in verticale
                            LV_FLEX_ALIGN_START); // allinea se più righe
      lv_obj_set_style_pad_row(titleLabel, 10, 0);

      // ---- CONTAINER APP ----
      container = lv_obj_create(root);
      lv_obj_remove_style_all(container);
      lv_obj_set_width(container, lv_pct(100));
      lv_obj_set_flex_grow(container, 1);
      lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50);
      lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_style_pad_all(container, 10, 0);
        lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

      onCreate();
  }
};