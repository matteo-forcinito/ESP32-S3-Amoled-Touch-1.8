#pragma once
#include "Navigation.h"

#include "SetTimeScreen.h"

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
                SetTimeNavigation::setTime();
            });
        });

        return appsList;
    }

  static void setTime() {
    Serial.print("Inizializzando l'ora..");
    WiFi.begin("WINDTRE-CEDF38 2.4GHz", "6djyuwd9mwf4sy9u");
    unsigned long wifiCheckTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiCheckTime < 6000) {
      Serial.print(".");
      delay(200);
      //wifiCheckTime = millis();
    }
    if(WiFi.status() != WL_CONNECTED) {
      wifiCheckTime = millis();
      WiFi.begin("MERCUSYS_7635", "34067642");
      while (WiFi.status() != WL_CONNECTED && millis() - wifiCheckTime < 6000) {
        Serial.print(".");
        delay(200);
        //wifiCheckTime = millis();
      }
    }
    if(WiFi.status() != WL_CONNECTED) {
      //printMessage("impossibile connettersi ad internet..");
      Serial.println("Cannot connect to wifi");
      delay(1000);
      
      return;
    }
    //Serial.println("WiFi connected");
    
    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    
    struct tm timeinfo;
    unsigned long syncStart = millis();
    // Aspetta fino a 10 secondi la sincronizzazione
    while (!getLocalTime(&timeinfo) && millis() - syncStart < 1000) {
      //Serial.println("Waiting for NTP sync...");
      //delay(500);
    }

    if (!getLocalTime(&timeinfo)) {
      //Serial.println("❌ Time sync failed");
      //printMessage("Impossibile inizializzare l'ora");
      delay(1000);
    } else {
      //Serial.printf("✅ Time set: %02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min);
      //printMessage("Ora inizializzata");
      TimeManager::saveTime();
      delay(1000);

    }

    WiFi.disconnect(true);  // disattiva per risparmiare batteria
    WiFi.mode(WIFI_OFF);
  }
};
