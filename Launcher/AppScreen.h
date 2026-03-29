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

    AppScreen *modal = nullptr;
public:
    AppScreen() : root(nullptr) {}
    virtual ~AppScreen() { destroy(); }

    virtual void onCreate() = 0;   // da implementare nella sottoclasse
    virtual void onDestroy() {}    // opzionale
    virtual void onShow() {}       // quando la schermata è caricata
    virtual void onTouch(int32_t x, int32_t y, int32_t fingers) {}
    virtual void onLoop() {}

    virtual void loop() {
        if(modal) {
            modal->loop();
        } else if(root) {
            onLoop();
        }
    }

    // Crea la schermata e richiama onCreate()
    virtual void create() {
        root = lv_obj_create(NULL);
        lv_obj_set_width(root, lv_pct(100));
        lv_obj_set_height(root, lv_pct(100));
        onCreate();
    }

    void openModal(AppScreen *newModal) {
        if(!newModal) return;
        modal = newModal;
        if(modal) {
            modal->create();
            modal->show();
        }
    }

    void closeModal() {
        if(!modal) return;
        
        AppScreen *old = modal;
        modal = nullptr;

        create();
        show();
        
        lv_timer_t * t = lv_timer_create([](lv_timer_t * timer) {
          AppScreen* app = (AppScreen*) timer->user_data;
          if(app) {
            delete app;
          }
          lv_timer_del(timer); // elimina il timer
        }, 500, old);
    }

    // Mostra questa schermata
    void show() {
        if (root)
            lv_scr_load(root);
        onShow();
    }

    // Distrugge l'oggetto LVGL e chiama onDestroy()
    void destroy() {
        onDestroy();
        if (root) {
            lv_obj_del(root);
            root = nullptr;
        }
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

    AppScreen* getModal() { return modal; }
};
