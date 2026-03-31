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
            if(old) {
                old->destroy();  // ⚠️ solo LVGL objects

                // defer delete DOPO che LVGL ha finito
                lv_async_call([](void *data) {
                    AppScreen* app = static_cast<AppScreen*>(data);
                    delete app;
                }, old);
            }
        }
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
