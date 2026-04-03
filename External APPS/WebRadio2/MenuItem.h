#pragma once
#include <lvgl.h>
#include <String>

struct MenuItem {
    String name;
    String icon;
    lv_event_cb_t callback; // <-- questo rimane
    void* userData = nullptr;

    MenuItem() : name(), icon(), callback(nullptr) {}

    MenuItem(const String &n, const String &i, lv_event_cb_t cb)
        : name(n), icon(i), callback(cb) {}
};
