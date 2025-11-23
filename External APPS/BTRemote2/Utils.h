#pragma once
#include <lvgl.h>
#include <Arduino.h>

struct MboxContext {
    lv_obj_t *mbox;
    String *appName;
};

struct MboxContextWithCallback {
    lv_obj_t *mbox;
    String *appName;
    lv_event_cb_t originalCallback; // 👉 aggiunta qui
    lv_event_t *originalEvent;      // (opzionale, utile se vuoi passare lo stesso evento)
};
