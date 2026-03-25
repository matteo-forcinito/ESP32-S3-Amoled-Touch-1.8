#pragma once
#include "AppScreen.h"
#include "SDManager.h"
//#include "Arduino_GFX_Library.h"
#include "XPowersLib.h"
//#include "FlashScreen.h"
#include "FlashScreen.h"
#include "SetTimeScreen.h"
#include "AppInstallerScreen.h"
#include "ControlCenterScreen.h"
#include "PaginatorScreen.h"
#include "SetTimeNavigation.h"

#define PLACEHOLDER_PATH "S:/assets/icons/placeholder.bin"

extern XPowersPMU power; 
//extern Arduino_GFX *gfx;
extern SDManager sdManager;

class HomeScreen : public AppScreen {
protected:
    AppState id = APP_HOME;
    
    lv_obj_t *list; 
    //lv_obj_t *progressBar; 
    lv_obj_t *labelStatus;
    lv_obj_t *title;
    lv_obj_t *battery;
    unsigned long lastUpdate = 0;

    bool isFlash = false;
    bool setTimeRequested = false;
public:
    void onCreate() override {
      lv_obj_t *header = lv_obj_create(root);
      lv_obj_remove_style_all(header);
      lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
      lv_obj_set_size(header, 340, 40);  // larghezza uguale allo schermo
      lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

      // Imposto flex row per contenitore
      lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(header,
                            LV_FLEX_ALIGN_SPACE_BETWEEN,  // title a destra, battery a sinistra
                            LV_FLEX_ALIGN_CENTER,          // allinea verticalmente
                            LV_FLEX_ALIGN_CENTER);         // cross axis
      lv_obj_set_style_pad_all(header, 10, 0); // padding interno

      lv_obj_add_event_cb(header, [](lv_event_t *e) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_event_get_indev(e));
        //if (dir == LV_DIR_BOTTOM) {
            ScreenManager::get().changeScreen(new ControlCenterScreen());
            // Azione da eseguire
        //}
      }, LV_EVENT_CLICKED, NULL);

      // Label titolo
      
      title = lv_label_create(header);
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        lastUpdate = millis();
        char timeString[6]; // "HH:MM" + terminatore null
        strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);
        lv_label_set_text(title, timeString);
      } else {
        lv_label_set_text(title, "App Launcher");
      }

      // Label batteria
      battery = lv_label_create(header);
      int batteryPercent = power.getBatteryPercent();
      lv_label_set_text_fmt(battery, "%d%%", batteryPercent);
      lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

      list = lv_obj_create(root);
      lv_obj_remove_style_all(list);
      lv_obj_set_size(list, 340, 350);
      lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);

      // Imposto layout flex
      lv_obj_set_flex_flow(list, LV_FLEX_FLOW_ROW_WRAP);  // Row + wrap
      lv_obj_set_flex_align(list,
                            LV_FLEX_ALIGN_START,  // allinea gli elementi all'inizio in orizzontale
                            LV_FLEX_ALIGN_START,  // allinea all'inizio in verticale
                            LV_FLEX_ALIGN_START); // allinea se più righe

      lv_obj_add_flag(list, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_pad_all(list, 0, 0);
      lv_obj_set_style_pad_row(list, 0, 0);
      lv_obj_set_style_pad_column(list, 0, 0);
      lv_obj_set_scroll_dir(list, LV_DIR_VER);

      labelStatus = lv_label_create(root);
      lv_label_set_text(labelStatus, "Ready");
      lv_obj_align(labelStatus, LV_ALIGN_BOTTOM_MID, 0, -10);

      loadAppList();
    }

    void loop() override {
      if(lastUpdate == 0 || millis() - lastUpdate > 50000) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
          lastUpdate = millis();
          char timeString[6]; // "HH:MM" + terminatore null
          strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);
          lv_label_set_text(title, timeString);
          int batteryPercent = power.getBatteryPercent();
          lv_label_set_text_fmt(battery, "%d%%", batteryPercent);
        }
      }
    }

    void flashlightApp() {

      // ---- TORCIA ----
      lv_obj_t *cont = lv_obj_create(list);
      lv_obj_remove_style_all(cont);
      lv_obj_set_width(cont, lv_pct(50));
      lv_obj_set_height(cont, LV_SIZE_CONTENT);
      lv_obj_set_style_radius(cont, 12, 0);
      lv_obj_set_style_bg_color(cont, lv_color_hex(0x1E1E1E), 0);
      lv_obj_set_style_bg_opa(cont, LV_OPA_80, 0);
      lv_obj_set_style_pad_all(cont, 8, 0);
      lv_obj_set_style_border_color(cont, lv_color_hex(0x333333), 0);
      lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

      // Fallback: simbolo LVGL

      lv_obj_t *icon = lv_img_create(cont);
      String iconPath = "/assets/icons/flashlight.bin";
      if (SD_MMC.exists(iconPath)) {
          String lvPath = "S:" + iconPath; // driver SD registrato in LVGL
          lv_img_set_src(icon, lvPath.c_str());
      } else {
          lv_img_set_src(icon, PLACEHOLDER_PATH);
      }
      lv_obj_set_size(icon, 64, 64);

      lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);

      // ---- NOME APP ----
      lv_obj_t *label = lv_label_create(cont);
      lv_label_set_text(label, "Torcia");
      lv_obj_set_style_text_color(label, lv_color_white(), 0);
      lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_width(label, LV_SIZE_CONTENT);
      lv_obj_set_style_pad_top(label, 6, 0);

      lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(cont, [](lv_event_t * e) {
        static const char *btns[] = {"Si", NULL};
        lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Conferma", "Vuoi accendere il flash?", btns, true);
        lv_obj_center(mbox);
        lv_obj_add_event_cb(mbox, [](lv_event_t * e) {
          lv_event_code_t code = lv_event_get_code(e);
          if (code == LV_EVENT_VALUE_CHANGED) {
            lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(e);
            const char *txt = lv_msgbox_get_active_btn_text(mbox);
            if (!txt) return;
            if (strcmp(txt, "Si") == 0) { 
              ScreenManager::get().changeScreen(new FlashScreen());
            }
            lv_obj_del(mbox);
          }
        }, LV_EVENT_ALL, mbox);
      }, LV_EVENT_CLICKED, nullptr);
    }

    void setTimeApp() {

      // ---- TORCIA ----
      lv_obj_t *cont = lv_obj_create(list);
      lv_obj_remove_style_all(cont);
      lv_obj_set_width(cont, lv_pct(50));
      lv_obj_set_height(cont, LV_SIZE_CONTENT);
      lv_obj_set_style_radius(cont, 12, 0);
      lv_obj_set_style_bg_color(cont, lv_color_hex(0x1E1E1E), 0);
      lv_obj_set_style_bg_opa(cont, LV_OPA_80, 0);
      lv_obj_set_style_pad_all(cont, 8, 0);
      lv_obj_set_style_border_color(cont, lv_color_hex(0x333333), 0);
      lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

      // Fallback: simbolo LVGL

      lv_obj_t *icon = lv_img_create(cont);
      String iconPath = "/assets/icons/clock.bin";
      if (SD_MMC.exists(iconPath)) {
          String lvPath = "S:" + iconPath; // driver SD registrato in LVGL
          lv_img_set_src(icon, lvPath.c_str());
      } else {
          lv_img_set_src(icon, PLACEHOLDER_PATH);
      }
      lv_obj_set_size(icon, 64, 64);

      lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);

      // ---- NOME APP ----
      lv_obj_t *label = lv_label_create(cont);
      lv_label_set_text(label, "Set Time");
      lv_obj_set_style_text_color(label, lv_color_white(), 0);
      lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_width(label, LV_SIZE_CONTENT);
      lv_obj_set_style_pad_top(label, 6, 0);

      lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(cont, [](lv_event_t * e) {
        static const char *btns[] = {"Si", NULL};
        lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Conferma", "Vuoi configurare l'ora tramite internet?", btns, true);
        lv_obj_center(mbox);
        lv_obj_add_event_cb(mbox, [](lv_event_t * e) {
          lv_event_code_t code = lv_event_get_code(e);
          if (code == LV_EVENT_VALUE_CHANGED) {
            lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(e);
            const char *txt = lv_msgbox_get_active_btn_text(mbox);
            if (!txt) return;
            if (strcmp(txt, "Si") == 0) { 
              //ScreenManager::get().changeScreen(new SetTimeScreen());
              SetTimeNavigation stn;
              stn.open();
            }
            lv_obj_del(mbox);
          }
        }, LV_EVENT_ALL, mbox);
      }, LV_EVENT_CLICKED, nullptr);
    }

    void controlCenterApp() {

      // ---- TORCIA ----
      lv_obj_t *cont = lv_obj_create(list);
      lv_obj_remove_style_all(cont);
      lv_obj_set_width(cont, lv_pct(50));
      lv_obj_set_height(cont, LV_SIZE_CONTENT);
      lv_obj_set_style_radius(cont, 12, 0);
      lv_obj_set_style_bg_color(cont, lv_color_hex(0x1E1E1E), 0);
      lv_obj_set_style_bg_opa(cont, LV_OPA_80, 0);
      lv_obj_set_style_pad_all(cont, 8, 0);
      lv_obj_set_style_border_color(cont, lv_color_hex(0x333333), 0);
      lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

      // Fallback: simbolo LVGL

      lv_obj_t *icon = lv_img_create(cont);
      String iconPath = "/assets/icons/control.bin";
      if (SD_MMC.exists(iconPath)) {
          String lvPath = "S:" + iconPath; // driver SD registrato in LVGL
          lv_img_set_src(icon, lvPath.c_str());
      } else {
          lv_img_set_src(icon, PLACEHOLDER_PATH);
      }
      lv_obj_set_size(icon, 64, 64);

      lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);

      // ---- NOME APP ----
      lv_obj_t *label = lv_label_create(cont);
      lv_label_set_text(label, "Control Center");
      lv_obj_set_style_text_color(label, lv_color_white(), 0);
      lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_width(label, LV_SIZE_CONTENT);
      lv_obj_set_style_pad_top(label, 6, 0);

      lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(cont, [](lv_event_t * e) {
        static const char *btns[] = {"Si", NULL};
        lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Conferma", "Vuoi aprire ControlCenter?", btns, true);
        lv_obj_center(mbox);
        lv_obj_add_event_cb(mbox, [](lv_event_t * e) {
          lv_event_code_t code = lv_event_get_code(e);
          if (code == LV_EVENT_VALUE_CHANGED) {
            lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(e);
            const char *txt = lv_msgbox_get_active_btn_text(mbox);
            if (!txt) return;
            if (strcmp(txt, "Si") == 0) { 
              ScreenManager::get().changeScreen(new ControlCenterScreen());
            }
            lv_obj_del(mbox);
          }
        }, LV_EVENT_ALL, mbox);
      }, LV_EVENT_CLICKED, nullptr);
    }

    static void cb_test(lv_event_t *e) {
    Serial.println("Clicked item");
}


// cb_openApp: ora legge String* da user_data, la usa e la cancella
static void cb_openApp(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    // user_data è la String* che abbiamo passato
    String *pName = static_cast<String*>(lv_event_get_user_data(e));
    if (!pName) return;

    const char *appName = pName->c_str();
    Serial.printf("[HomeScreen] 📦 Apertura app: %s\n", appName);

    // Creiamo il msgbox: passiamo ctx che contiene la copy, oppure creiamo un nuovo ctx
    static const char *btns[] = {"Si", "No", NULL};
    String mboxDescr = "Vuoi aprire l'app " + *pName + "?";
    lv_obj_t *parent = lv_obj_get_screen(lv_event_get_target(e));
    lv_obj_t *mbox = lv_msgbox_create(parent, "Conferma", mboxDescr.c_str(), btns, true);

    //lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Conferma", mboxDescr.c_str(), btns, true);
    lv_obj_center(mbox);
    lv_obj_move_foreground(mbox);

    // Creiamo un contesto che mantiene la String* per il callback del msgbox
    struct Ctx { lv_obj_t* mbox; String *name; };
    Ctx *ctx = new Ctx{mbox, pName}; // trasferiamo ownership di pName al ctx

    // callback del msgbox: user_data = ctx
    lv_obj_add_event_cb(mbox, [](lv_event_t *ev) {
        lv_event_code_t c = lv_event_get_code(ev);
        if (c != LV_EVENT_VALUE_CHANGED) return;

        Ctx *ctx = static_cast<Ctx*>(lv_event_get_user_data(ev));
        if (!ctx) return;

        const char *txt = lv_msgbox_get_active_btn_text(ctx->mbox);
        if (txt && strcmp(txt, "Si") == 0) {
            Serial.printf("[HomeScreen] 🚀 Avvio app %s\n", ctx->name->c_str());
            ScreenManager::get().changeScreen(new AppInstallerScreen(*(ctx->name)));
        }

        // cleanup
        lv_obj_del(ctx->mbox);
        delete ctx->name; // libera la String allocata precedentemente
        delete ctx;
    }, LV_EVENT_VALUE_CHANGED, ctx);
}



static std::vector<MenuItem> externalApps() {
    std::vector<MenuItem> appsList;

    if (!sdManager.isReady()) {
        Serial.println("[HomeScreen] ⚠️ SD non inizializzata");
        return appsList;
    }

    auto apps = sdManager.listAppFolders("/apps");
    if (apps.empty()) {
        Serial.println("[HomeScreen] Nessuna app trovata");
        return appsList;
    }

    for (auto &appName : apps) {
        String appPath = "/apps/" + appName;
        String iconPath = appPath + "/icon.bin";

        // Determina l’icona (path o placeholder)
        String icon;
        if (SD_MMC.exists(iconPath.c_str())) {
            icon = "S:" + iconPath; // Usa driver SD per LVGL
            Serial.printf("[externalApps] 🖼️ Icona trovata per %s\n", appName.c_str());
        } else {
            icon = String(PLACEHOLDER_PATH);
            Serial.printf("[externalApps] 🖼️ Icona default per %s\n", appName.c_str());
        }

        // Aggiunge il MenuItem alla lista
        appsList.emplace_back(appName, icon, HomeScreen::cb_openApp);
    }

    return appsList;
}


    void externalApp() {

      // ---- TORCIA ----
      lv_obj_t *cont = lv_obj_create(list);
      lv_obj_remove_style_all(cont);
      lv_obj_set_width(cont, lv_pct(50));
      lv_obj_set_height(cont, LV_SIZE_CONTENT);
      lv_obj_set_style_radius(cont, 12, 0);
      lv_obj_set_style_bg_color(cont, lv_color_hex(0x1E1E1E), 0);
      lv_obj_set_style_bg_opa(cont, LV_OPA_80, 0);
      lv_obj_set_style_pad_all(cont, 8, 0);
      lv_obj_set_style_border_color(cont, lv_color_hex(0x333333), 0);
      lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

      // Fallback: simbolo LVGL

      lv_obj_t *icon = lv_img_create(cont);
      String iconPath = "/assets/icons/control.bin";
      if (SD_MMC.exists(iconPath)) {
          String lvPath = "S:" + iconPath; // driver SD registrato in LVGL
          lv_img_set_src(icon, lvPath.c_str());
      } else {
          lv_img_set_src(icon, PLACEHOLDER_PATH);
      }
      lv_obj_set_size(icon, 64, 64);

      lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);

      // ---- NOME APP ----
      lv_obj_t *label = lv_label_create(cont);
      lv_label_set_text(label, "External APP");
      lv_obj_set_style_text_color(label, lv_color_white(), 0);
      lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_width(label, LV_SIZE_CONTENT);
      lv_obj_set_style_pad_top(label, 6, 0);

      lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(cont, [](lv_event_t * e) {
        static const char *btns[] = {"Si", NULL};
        lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Conferma", "Vuoi aprire ExternalApp?", btns, true);
        lv_obj_center(mbox);
        lv_obj_add_event_cb(mbox, [](lv_event_t * e) {
          lv_event_code_t code = lv_event_get_code(e);
          if (code == LV_EVENT_VALUE_CHANGED) {
            lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(e);
            const char *txt = lv_msgbox_get_active_btn_text(mbox);
            if (!txt) return;
            if (strcmp(txt, "Si") == 0) { 
              auto *apps = new std::vector<MenuItem>(HomeScreen::externalApps());
              ScreenManager::get().changeScreen(new PaginatorScreen("Test Page", *apps));
            }
            lv_obj_del(mbox);
          }
        }, LV_EVENT_ALL, mbox);
      }, LV_EVENT_CLICKED, nullptr);
    }

void loadSystemApps() {
  externalApp();
  controlCenterApp();
  setTimeApp();
  flashlightApp();
}

void loadAppList() {


  lv_obj_clean(list);

  loadSystemApps();

    if (!sdManager.isReady()) {
        Serial.println("[HomeScreen] ⚠️ SD non inizializzata");
        lv_label_set_text(labelStatus, "SD non pronta");
        return;
    }

    auto apps = sdManager.listAppFolders("/apps");
    if (apps.empty()) {
        Serial.println("[HomeScreen] Nessuna app trovata");
        lv_label_set_text(labelStatus, "Nessuna app trovata");
        return;
    }

    for (auto &appName : apps) {
        String appPath = "/apps/" + appName;
        String binPath = appPath + "/" + appName + ".bin";
        String iconPath = appPath + "/icon.bin";

        Serial.printf("[HomeScreen] App trovata: %s\n", appName.c_str());

        // Crea il container per ogni app
        lv_obj_t *cont = lv_obj_create(list);
        lv_obj_remove_style_all(cont);
        lv_obj_set_width(cont, lv_pct(50));
        lv_obj_set_height(cont, LV_SIZE_CONTENT);
        lv_obj_set_style_radius(cont, 12, 0);
        lv_obj_set_style_bg_color(cont, lv_color_hex(0x1E1E1E), 0);
        lv_obj_set_style_bg_opa(cont, LV_OPA_80, 0);
        lv_obj_set_style_pad_all(cont, 8, 0);
        lv_obj_set_style_border_color(cont, lv_color_hex(0x333333), 0);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // ---- ICONA ----
        lv_obj_t *icon = lv_img_create(cont);

        if (SD_MMC.exists(iconPath.c_str())) {
            String lvPath = "S:" + iconPath; // driver SD registrato in LVGL
            lv_img_set_src(icon, lvPath.c_str());
            Serial.printf("[HomeScreen] 🖼️ Icona personalizzata: %s\n", iconPath.c_str());
        } else {
            lv_img_set_src(icon, PLACEHOLDER_PATH);
            Serial.printf("[HomeScreen] 🖼️ Icona default per %s\n", appName.c_str());
        }
        lv_obj_set_size(icon, 64, 64);

        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);

        // ---- NOME APP ----
        lv_obj_t *label = lv_label_create(cont);
        lv_label_set_text(label, appName.c_str());
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(label, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_top(label, 6, 0);
String *appNamePtr = new String(appName);
Serial.println("[DEBUG] Created appNamePtr with value: " + *appNamePtr);

lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
lv_obj_add_event_cb(cont, [](lv_event_t * e) {
    String *appName = static_cast<String *>(lv_event_get_user_data(e));

    static const char *btns[] = {"Si", "No", NULL};
    String mboxDescr = "Vuoi aprire l'app " + *appName + "?";

    lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Conferma", mboxDescr.c_str(), btns, true);
    lv_obj_center(mbox);

    // allochiamo un piccolo contesto per la callback
    MboxContext *ctx = new MboxContext{mbox, new String(*appName)};

    lv_obj_add_event_cb(mbox, [](lv_event_t * e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_VALUE_CHANGED) {
            MboxContext *ctx = static_cast<MboxContext *>(lv_event_get_user_data(e));
            const char *txt = lv_msgbox_get_active_btn_text(ctx->mbox);

            if (!txt) return;

            if (strcmp(txt, "Si") == 0) {
                Serial.printf("Apro l'app: %s\n", ctx->appName->c_str());
                ScreenManager::get().changeScreen(new AppInstallerScreen(*(ctx->appName)));
            }

            // pulizia
            lv_obj_del(ctx->mbox);
            delete ctx->appName;
            delete ctx;
        }
    }, LV_EVENT_VALUE_CHANGED, ctx);

}, LV_EVENT_CLICKED, appNamePtr);

    }

    lv_label_set_text(labelStatus, "App trovate!");
}

static void mbox_event_handler(lv_event_t* e2) {
    Serial.println("mbox_event_handler");
    String* appName = (String*)lv_event_get_user_data(e2);
    Serial.println("mbox_event_handler::got app name");
    const char* btnTxt = lv_msgbox_get_active_btn_text(lv_event_get_target(e2));
    Serial.println("mbox_event_handler::got btn text");
    if (btnTxt && strcmp(btnTxt, "Si") == 0) {
        Serial.println("Apro l'app: " + *appName);
        ScreenManager::get().changeScreen(new AppInstallerScreen(*(appName)));
    }
}


};
