#pragma once
#include <lvgl.h>
#include "HWCDC.h"

#include "Utils.h"
#include "ScreenManager.h"
#include "AppState.h"

extern HWCDC USBSerial;

class AppScreen {
protected:
    lv_obj_t* root;  // oggetto principale LVGL di questa schermata
    AppState id = APP_INVALID_APP;

    bool requestedTime = false;
public:
    AppScreen() : root(nullptr) {}
    virtual ~AppScreen() { destroy(); }

    virtual void onCreate() = 0;   // da implementare nella sottoclasse
    virtual void onDestroy() {}    // opzionale
    virtual void onShow() {}       // quando la schermata è caricata
    virtual void onTouch(int32_t x, int32_t y, int32_t fingers) {}
    virtual void onLoop() {}

    void loop() {
        if(root) {
            onLoop();
        }
    }

    // Crea la schermata e richiama onCreate()
    void create() {
        root = lv_obj_create(NULL);
        lv_obj_set_width(root, lv_pct(100));
        lv_obj_set_height(root, lv_pct(100));
        onCreate();
    }

    // Mostra questa schermata
    void show() {
        if (root)
            lv_scr_load(root);
        onShow();
    }

    // Distrugge l'oggetto LVGL e chiama onDestroy()
    void destroy() {
        if (root) {
            lv_obj_del(root);
            root = nullptr;
        }
        onDestroy();
    }

    void touch(int32_t x, int32_t y, int32_t fingers) {
        onTouch(x, y, fingers);
    }

    AppState getId() const {
        return id;
    }

    void setRequestedTime(bool isRequested) {
        requestedTime = isRequested;
    }

    bool isRequestedTime() {
        return requestedTime;
    }
};
