#pragma once
#include "Navigation.h"
#include "WebRadioScreen.h"
#include "WiFiScreen.h"
#include "RadioStationsScreen.h"

class HomeNavigation : public Navigation {
public:
    const char *getTitle() const override { return "Home"; }

    std::vector<MenuItem> buildAppList() override {
        std::vector<MenuItem> appsList;

        appsList.emplace_back("Web Radio", "S:/assets/icons/radio.bin", [](lv_event_t *e) {
            ScreenManager::get().changeScreen(new WebRadioScreen());
        });

        appsList.emplace_back("WiFi", "S:/assets/icons/wifi.bin", [](lv_event_t *e) {
            ScreenManager::get().changeScreen(new WiFiScreen());
        });

        appsList.emplace_back("Stations", "S:/assets/icons/boombox.bin", [](lv_event_t *e) {
            ScreenManager::get().changeScreen(new RadioStationsScreen());
        });

        appsList.emplace_back("Back To Launcher", "S:/assets/icons/back-launcher.bin", [](lv_event_t *e) {
            //ScreenManager::get().changeScreen(new RadioStationsScreen());
        });

        return appsList;
    }
};
