#pragma once
#include "AppScreen.h"
#include "MenuItem.h"
#include <vector>
#include <string>
#include "esp_heap_caps.h"
#include "SDManager.h"
#include "ControlCenterScreen.h"

extern SensorPCF85063 rtc;
extern XPowersPMU power;
extern SDManager sdManager;

class PaginatorScreen : public AppScreen {
private:
    std::string title;
    std::vector<MenuItem> items;
    int currentPage = 0;
    int itemsPerPage = 6;

    lv_obj_t *titleLabel = nullptr;
    lv_obj_t *timeLabel = nullptr;
    lv_obj_t *chargingLabel = nullptr;
    lv_obj_t *battery = nullptr;
    lv_obj_t *container = nullptr;
    lv_obj_t *pageLabel = nullptr;
    lv_obj_t *btnPrev = nullptr;
    lv_obj_t *btnNext = nullptr;
    lv_obj_t *footer = nullptr;
    lv_obj_t *lblRam = nullptr;
    lv_obj_t *connections = nullptr;
    lv_obj_t *wifiConnection = nullptr;

    bool isCharging = false;
    bool wifiConnected = false;
    unsigned long lastUpdate = 0;

    // --- Callback persistenti dei pulsanti di navigazione ---
    struct NavCallbackHolder {
        std::function<void(lv_event_t*)> cb;
    };
    std::vector<NavCallbackHolder> navCallbacks;
    // --- membri aggiuntivi per sicurezza ---
    std::function<void(lv_event_t*)> prevCallback;
    std::function<void(lv_event_t*)> nextCallback;
    lv_obj_t* appButtonsContainer = nullptr; // container interno per i pulsanti delle app

    String getRamUsage() {
        size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        size_t totalHeap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
        size_t usedHeap = totalHeap - freeHeap;
        char buf[64];
        sprintf(buf, "%dKB / %dKB (%d%%)",
                usedHeap / 1024, totalHeap / 1024, (usedHeap * 100) / totalHeap);
        return String(buf);
    }

public:
    PaginatorScreen(const std::string &title, std::vector<MenuItem> items)
        : title(title), items(std::move(items)) {}

    void onCreate() override {
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(root, 10, 0);
        lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);

        // --- HEADER ---
        // (qui puoi lasciare il codice dell'header come prima: timeLabel, connections, lblRam, wifiConnection, battery...)
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

        // --- TITOLO ---
        titleLabel = lv_label_create(root);
        lv_obj_set_width(titleLabel, lv_pct(100));
        lv_label_set_text(titleLabel, title.c_str());
        lv_obj_set_style_text_color(titleLabel, lv_color_white(), 0);
        lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_24, 0);
        lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);
/*
        // --- CONTAINER APPLICAZIONI ---
        container = lv_obj_create(root);
        lv_obj_remove_style_all(container);
        lv_obj_set_width(container, lv_pct(100));
        lv_obj_set_flex_grow(container, 1);
        lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_add_flag(container, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(container, LV_DIR_VER);
*/
        // container interno solo per i pulsanti delle app
        appButtonsContainer = lv_obj_create(root);
        lv_obj_remove_style_all(appButtonsContainer);
        lv_obj_set_width(appButtonsContainer, lv_pct(100));
        lv_obj_set_flex_flow(appButtonsContainer, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_grow(appButtonsContainer, 1);
        lv_obj_set_flex_align(appButtonsContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

        // --- FOOTER ---
        footer = lv_obj_create(root);
        lv_obj_remove_style_all(footer);
        lv_obj_set_width(footer, lv_pct(100));
        lv_obj_set_height(footer, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_BETWEEN);

        // --- CALLBACK PULSANTI NAVIGAZIONE ---
        prevCallback = [this](lv_event_t *) {
            if(currentPage > 0) {
                currentPage--;
                renderPage();
            }
        };

        nextCallback = [this](lv_event_t *) {
            if((currentPage + 1) * itemsPerPage < (int)items.size()) {
                currentPage++;
                renderPage();
            }
        };

        // --- CREAZIONE PULSANTI NAV ---
        btnPrev = createNavButton(footer, LV_SYMBOL_LEFT, prevCallback);
        
        pageLabel = lv_label_create(footer);
        lv_label_set_text(pageLabel, "");
        lv_obj_set_style_text_color(pageLabel, lv_color_white(), 0);

        btnNext = createNavButton(footer, LV_SYMBOL_RIGHT, nextCallback);


        // Render iniziale
        renderPage();
    }

    void onLoop() override {
        RTC_DateTime datetime = rtc.getDateTime();
        int seconds = datetime.second;

        if(power.isCharging() != isCharging) {
            isCharging = power.isCharging();
            if(isCharging) lv_obj_clear_flag(chargingLabel, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(chargingLabel, LV_OBJ_FLAG_HIDDEN);
        }

        if (seconds == 0 && (lastUpdate == 0 || millis() - lastUpdate >= 1000)) update();

        if(WiFi.getMode() != WIFI_MODE_NULL && !wifiConnected) {
            lv_obj_clear_flag(wifiConnection, LV_OBJ_FLAG_HIDDEN);
            wifiConnected = true;
        }
        if(WiFi.getMode() == WIFI_MODE_NULL && wifiConnected) {
            lv_obj_add_flag(wifiConnection, LV_OBJ_FLAG_HIDDEN);
            wifiConnected = false;
        }
    }

    void onDestroy() override {
        /*
        for (auto &item : items) {
            if (item.userData) { free(item.userData); item.userData = nullptr; }
        }
        */
        navCallbacks.clear();
    }

private:

    void update() {
        RTC_DateTime datetime = rtc.getDateTime();
        lastUpdate = millis();

        char timeString[6];
        sprintf(timeString, "%02d:%02d", datetime.hour, datetime.minute);
        lv_label_set_text(timeLabel, timeString);

        int batteryPercent = power.getBatteryPercent();
        lv_label_set_text_fmt(battery, "%d%%", batteryPercent);
        lv_label_set_text(lblRam, getRamUsage().c_str());
    }

    void renderPage() {
        // pulisco solo i pulsanti delle app
        lv_obj_clean(appButtonsContainer);

        int start = currentPage * itemsPerPage;
        int end = std::min(start + itemsPerPage, (int)items.size());

        // visibilità footer
        if (items.size() <= itemsPerPage) lv_obj_add_flag(footer, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(footer, LV_OBJ_FLAG_HIDDEN);

        // --- pulsanti/app ---
        for (int i = start; i < end; ++i) {
            auto &item = items[i];
            lv_obj_t *btn = lv_btn_create(appButtonsContainer);
            lv_obj_remove_style_all(btn);
            lv_obj_set_width(btn, lv_pct(50));
            lv_obj_set_height(btn, LV_SIZE_CONTENT);
            lv_obj_set_style_radius(btn, 12, 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E1E1E), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0);
            lv_obj_set_style_pad_all(btn, 8, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x333333), 0);
            lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            lv_obj_t *icon;
            if (strncmp(item.icon.c_str(), "S:", 2) == 0) {
                String iconPath = "S:/assets/icons/placeholder.bin";
                if(sdManager.isPathExists(item.icon.substring(2).c_str())) iconPath = item.icon;
                icon = lv_img_create(btn);
                lv_img_set_src(icon, iconPath.c_str());
                lv_obj_set_size(icon, 64, 64);
            } else {
                icon = lv_label_create(btn);
                lv_label_set_text(icon, item.icon.c_str());
                lv_obj_set_style_text_color(icon, lv_color_white(), 0);
                lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
            }
            lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 5);

            lv_obj_t *nameLbl = lv_label_create(btn);
            lv_label_set_text(nameLbl, item.name.c_str());
            lv_obj_set_style_text_color(nameLbl, lv_color_white(), 0);
            lv_obj_align(nameLbl, LV_ALIGN_BOTTOM_MID, 0, -5);

            if(item.callback) {
                lv_obj_add_event_cb(btn, item.callback, LV_EVENT_CLICKED, item.userData ? item.userData : (void*)&item);
            }
        }

        // aggiornamento label pagina
        int totalPages = (items.size() + itemsPerPage - 1) / itemsPerPage;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d / %d", currentPage + 1, totalPages);
        lv_label_set_text(pageLabel, buf);

        // stato pulsanti navigazione
        lv_obj_set_style_opa(btnPrev, currentPage==0 ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
        lv_obj_set_style_opa(btnNext, end >= (int)items.size() ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
        if(currentPage == 0) lv_obj_add_state(btnPrev, LV_STATE_DISABLED);
        else lv_obj_clear_state(btnPrev, LV_STATE_DISABLED);
        if(end >= (int)items.size()) lv_obj_add_state(btnNext, LV_STATE_DISABLED);
        else lv_obj_clear_state(btnNext, LV_STATE_DISABLED);
    }

lv_obj_t *createNavButton(lv_obj_t *parent, const char *symbol,
                          std::function<void(lv_event_t*)> callback) {
    // Allocazione heap per lifetime stabile
    auto cb_ptr = new std::function<void(lv_event_t*)>(callback);

    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_height(btn, 40);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    lv_obj_add_event_cb(btn, [](lv_event_t *e){
        auto f = static_cast<std::function<void(lv_event_t*)>*>(lv_event_get_user_data(e));
        if(f) (*f)(e);
    }, LV_EVENT_CLICKED, cb_ptr);

    lv_obj_add_event_cb(btn, [](lv_event_t *e){
        auto f = static_cast<std::function<void(lv_event_t*)>*>(lv_event_get_user_data(e));
        if(f) delete f;
    }, LV_EVENT_DELETE, cb_ptr);

    return btn;
}
};