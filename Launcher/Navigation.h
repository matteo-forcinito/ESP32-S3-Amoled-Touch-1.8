#pragma once
#include <vector>
#include <Arduino.h>
#include "ScreenManager.h"
#include "PaginatorScreen.h"
#include "MenuItem.h"
#include "Utils.h"

class Navigation {
public:
    virtual ~Navigation() = default;

    virtual const char *getTitle() const = 0;
    virtual std::vector<MenuItem> buildAppList() = 0;

    void open() {
        auto appList = buildAppList();
        if (appList.empty()) return;

        ScreenManager::get().changeScreen(new PaginatorScreen(getTitle(), appList));
    }
};
