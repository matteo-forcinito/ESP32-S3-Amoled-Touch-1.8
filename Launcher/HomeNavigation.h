#pragma once
#include "Navigation.h"
#include "SystemAppsNavigation.h"
#include "ExternalAppsNavigation.h"

class HomeNavigation : public Navigation {
public:
    const char *getTitle() const override { return "Home"; }

    std::vector<MenuItem> buildAppList() override {
        std::vector<MenuItem> appsList;

        appsList.emplace_back("System Apps", "S:/assets/icons/settings.bin", [](lv_event_t *e) {
            SystemAppsNavigation nav;
            nav.open();
        });

        appsList.emplace_back("External Apps", "S:/assets/icons/external.bin", [](lv_event_t *e) {
            ExternalAppsNavigation nav;
            nav.open();
        });

        return appsList;
    }
};
