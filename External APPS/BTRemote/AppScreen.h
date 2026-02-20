#pragma once
#include <lvgl.h>

#include "ScreenManager.h"

class AppScreen {
private:
    lv_obj_t *settingsContainer;
protected:
    lv_obj_t* root;  // oggetto principale LVGL di questa schermata
    lv_obj_t  *container;
    lv_obj_t* settings;

    bool requestedTime = false;
    bool isSettings = false;
public:
    AppScreen() : root(nullptr) {}
    virtual ~AppScreen() { destroy(); }

    virtual void onCreate() = 0;   // da implementare nella sottoclasse
    virtual void onDestroy() {}    // opzionale
    virtual void onShow() {}       // quando la schermata è caricata
    virtual void onTouch(int32_t x, int32_t y, int32_t fingers) {}
    virtual void onTouchReleased() {}
    //virtual void onButtonPressed() {}
    virtual void onLoop() {}

    // Crea la schermata e richiama onCreate()
    void create() {
        root = lv_obj_create(NULL);
        lv_obj_set_width(root, lv_pct(100));
        lv_obj_set_height(root, lv_pct(100));
        isSettings = false;
        container = lv_obj_create(root);
        lv_obj_set_width(container, lv_pct(100));
        lv_obj_set_height(container, lv_pct(100));

        settingsContainer = lv_obj_create(root);
        lv_obj_remove_style_all(settingsContainer);
        lv_obj_set_width(settingsContainer, lv_pct(100));
        lv_obj_set_height(settingsContainer, lv_pct(100));
        lv_obj_add_flag(settingsContainer, LV_OBJ_FLAG_HIDDEN);
        //lv_obj_clear_flag(settings, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t *labelSettings = lv_label_create(settingsContainer);
        lv_label_set_text(labelSettings, "Settings");
        settings = lv_obj_create(settingsContainer);
        lv_obj_set_width(settings, lv_pct(100));
        lv_obj_set_height(settings, lv_pct(100));

        lv_obj_t *actions = lv_obj_create(settingsContainer);
        lv_obj_set_width(actions, lv_pct(100));
        lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
        lv_obj_t *btnRestart = lv_btn_create(actions);
        lv_obj_t *labelBtnRestart = lv_label_create(btnRestart);
        lv_label_set_text(labelBtnRestart, "Restart");
        lv_obj_set_flex_grow(labelBtnRestart, 1);
        lv_obj_t *btnTurnOff = lv_btn_create(actions);
        lv_obj_t *labelBtnTurnOff = lv_label_create(btnTurnOff);
        lv_label_set_text(labelBtnTurnOff, "Restart");
        lv_obj_set_flex_grow(labelBtnTurnOff, 1);
        

        lv_obj_add_event_cb(btnRestart, [](lv_event_t *e) {
            esp_restart();
        }, LV_EVENT_CLICKED, NULL);

        lv_obj_add_event_cb(btnTurnOff, [](lv_event_t *e) {
            esp_deep_sleep_start();
        }, LV_EVENT_CLICKED, NULL);

        onCreate();
    }

    // Mostra questa schermata
    void show() {
        if (root)
            lv_scr_load(root);
        onShow();
    }
    
    void loop() {
        if(!isSettings) {
            onLoop();
        }
    }

    // Distrugge l'oggetto LVGL e chiama onDestroy()
    void destroy() {
        if (root) {
            lv_obj_del(root);
            root = nullptr;
        }
        onDestroy();
    }

    void onButtonPressed() {
        isSettings = !isSettings;
        if(isSettings) {
            lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(settingsContainer, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(settingsContainer, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);
        }

        //lv_obj_clear_flag(settings, LV_OBJ_FLAG_HIDDEN);
        //onButtonPressed();
    }

    void touch(int32_t x, int32_t y, int32_t fingers) {
        if(!isSettings) {
            onTouch(x, y, fingers);
        }
    }

    void setRequestedTime(bool isRequested) {
        requestedTime = isRequested;
    }

    bool isRequestedTime() {
        return requestedTime;
    }
};
