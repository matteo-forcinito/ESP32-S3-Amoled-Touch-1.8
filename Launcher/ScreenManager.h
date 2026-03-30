#pragma once
#include "AppScreen.h"

class ScreenManager {
private:
    AppScreen* current = nullptr;

public:
    static ScreenManager& get() {
        static ScreenManager instance;
        return instance;
    }

    void changeScreen(AppScreen* next) {
        AppScreen* old = current;

        current = next;
        if (current) {  
            current->create();
            current->show();
        }
        lv_timer_t * t = lv_timer_create([](lv_timer_t * timer) {
          AppScreen* app = (AppScreen*) timer->user_data;
          if(app) {
            //app->destroy();
            delete app;
          }
          lv_timer_del(timer); // elimina il timer
        }, 50, old);
    }

    void openModal(AppScreen *app) {
        if(!app) return;
        if(current->getModal()) current->closeModal();

        current->openModal(app);
    }

    void loop() {
        if (current) current->loop();
    }

    void touch(int32_t x, int32_t y, int32_t fingers) {
        if(current) current->touch(x, y, fingers);
    }

    AppScreen* getCurrent() {
        return current;
    }
};
