#pragma once
#include "Navigation.h"
#include "SetTimeScreen.h"
#include "WiFiScreen.h"

extern SensorPCF85063 rtc;

class SetTimeNavigation : public Navigation {
public:
    const char *getTitle() const override { return "System Apps"; }


    static void onAppClick(lv_event_t *e, lv_event_cb_t originalCallback) {
        String *appName = static_cast<String *>(lv_event_get_user_data(e));

        static const char *btns[] = {"Si", "No", NULL};
        String mboxDescr = "Vuoi aprire l'app " + *appName + "?";

        lv_obj_t *mbox = lv_msgbox_create(lv_layer_top(), "Conferma", mboxDescr.c_str(), btns, true);
        lv_obj_center(mbox);

        // Creiamo il contesto con la callback originale
        MboxContextWithCallback *ctx = new MboxContextWithCallback{mbox, new String(*appName), originalCallback, e};

        lv_obj_add_event_cb(mbox, [](lv_event_t *ev) {
            lv_event_code_t code = lv_event_get_code(ev);
            if (code == LV_EVENT_VALUE_CHANGED) {
                MboxContextWithCallback *ctx = static_cast<MboxContextWithCallback *>(lv_event_get_user_data(ev));
                const char *txt = lv_msgbox_get_active_btn_text(ctx->mbox);
                if (!txt) return;

                if (strcmp(txt, "Si") == 0 && ctx->originalCallback) {
                    // Chiamiamo la callback originale 👇
                    ctx->originalCallback(ctx->originalEvent);
                }

                // Pulizia
                lv_obj_del(ctx->mbox);
                delete ctx->appName;
                delete ctx;
            }
        }, LV_EVENT_VALUE_CHANGED, ctx);
    }


    std::vector<MenuItem> buildAppList() override {
        std::vector<MenuItem> appsList;
        
        appsList.emplace_back("Set Manual", LV_SYMBOL_EDIT, [](lv_event_t *e) {
            SetTimeNavigation::onAppClick(e, [](lv_event_t *e){
                ScreenManager::get().changeScreen(new SetTimeScreen());
            });
        });

        appsList.emplace_back("Set Auto", LV_SYMBOL_WIFI, [](lv_event_t *e) {
            SetTimeNavigation::onAppClick(e, [](lv_event_t *e){
              if(WiFi.status() != WL_CONNECTED) {
                ScreenManager::get().openModal(new WiFiScreen());
              } else {
                SetTimeNavigation::setTime();
              }
            });
        });

        return appsList;
    }

  static void setTime() {
    //USBSerial("Inizializzando l'ora..");

    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    struct tm timeinfo;
    unsigned long syncStart = millis();

    while (!getLocalTime(&timeinfo) && millis() - syncStart < 10000) {
        delay(100);
    }

    if (!getLocalTime(&timeinfo)) {
        lv_obj_t *error = lv_msgbox_create(lv_layer_top(), "Attention", "Errore durante la configurazione!", NULL, true);
        lv_obj_center(error);
        return;
    }

    // ✅ Scrivi su RTC
    rtc.setDateTime(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
    );

    lv_obj_t *success = lv_msgbox_create(lv_layer_top(), "Attention", "Ora sincronizzata su RTC!", NULL, true);
    lv_obj_center(success);
  }
};
