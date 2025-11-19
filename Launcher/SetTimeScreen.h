#pragma once
#include "AppScreen.h"
#include "WiFi.h"
#include "TimeManager.h"
#include "ScreenManager.h"
#include <ctime>

extern bool backHome;

class SetTimeScreen : public AppScreen {
private:
    lv_obj_t *calendar;
    lv_obj_t *roller_hour;
    lv_obj_t *roller_min;
    lv_obj_t *saveBtn;
    lv_obj_t *cancelBtn;
    lv_calendar_date_t selectedDate = {0, 0, 0};

public:
    SetTimeScreen() = default;

    void onCreate() override {
        lv_obj_t *label = lv_label_create(root);
        lv_label_set_text(label, "Imposta Data e Ora");
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

        // ---- CALENDARIO ----
        calendar = lv_calendar_create(root);
        lv_obj_set_size(calendar, lv_pct(80), lv_pct(55));
        lv_obj_align(calendar, LV_ALIGN_TOP_MID, 0, 40);

        static const char *giorni_it[] = {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"};
        lv_calendar_set_day_names(calendar, giorni_it);

        // Imposta data di oggi (locale o fallback)
        struct tm timeinfo;
        int year = 2025, month = 1, day = 1;
        if (getLocalTime(&timeinfo)) {
            year = timeinfo.tm_year + 1900;
            month = timeinfo.tm_mon + 1;
            day = timeinfo.tm_mday;
        }

        // ⚙️ Versione corretta per LVGL 8.4.0
        lv_calendar_set_today_date(calendar, year, month, day);
        lv_calendar_set_showed_date(calendar, year, month);

        // Aggiungi navigazione mese/anno
        lv_calendar_header_dropdown_create(calendar);

        // Aggiungi callback per selezione data
        lv_obj_add_event_cb(calendar, calendar_event_handler, LV_EVENT_ALL, this);


        // ---- ROLLER ORA ----
        roller_hour = lv_roller_create(root);
        lv_roller_set_options(roller_hour,
            "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
            LV_ROLLER_MODE_INFINITE);
        lv_obj_set_width(roller_hour, 60);
        lv_obj_set_height(roller_hour, 100);
        lv_obj_align(roller_hour, LV_ALIGN_BOTTOM_LEFT, 40, -20);

        // ---- ROLLER MINUTI ----
        roller_min = lv_roller_create(root);
        lv_roller_set_options(roller_min,
            "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59",
            LV_ROLLER_MODE_INFINITE);
        lv_obj_set_width(roller_min, 60);
        lv_obj_set_height(roller_min, 100);
        lv_obj_align(roller_min, LV_ALIGN_BOTTOM_RIGHT, -40, -20);

        // ---- PULSANTE SALVA ----
        saveBtn = lv_btn_create(root);
        lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_LEFT, 40, -130);
        lv_obj_set_size(saveBtn, 100, 40);
        lv_obj_t *saveLbl = lv_label_create(saveBtn);
        lv_label_set_text(saveLbl, "Salva");
        lv_obj_center(saveLbl);
        lv_obj_add_event_cb(saveBtn, onSaveEvent, LV_EVENT_CLICKED, this);

        // ---- PULSANTE ANNULLA ----
        cancelBtn = lv_btn_create(root);
        lv_obj_align(cancelBtn, LV_ALIGN_BOTTOM_RIGHT, -40, -130);
        lv_obj_set_size(cancelBtn, 100, 40);
        lv_obj_t *cancelLbl = lv_label_create(cancelBtn);
        lv_label_set_text(cancelLbl, "Annulla");
        lv_obj_center(cancelLbl);
        lv_obj_add_event_cb(cancelBtn, onCancelEvent, LV_EVENT_CLICKED, this);
    }

    static void onSaveEvent(lv_event_t *e) {
        auto *screen = static_cast<SetTimeScreen *>(lv_event_get_user_data(e));
        screen->saveDateTime();
    }

    static void onCancelEvent(lv_event_t *e) {
        backHome = true;
    }

    void saveDateTime() {
        if (selectedDate.day == 0) {
            Serial.println("[SetTimeScreen] ⚠️ Nessuna data selezionata");
            return;
        }

        char ora_str[4], min_str[4];
        lv_roller_get_selected_str(roller_hour, ora_str, sizeof(ora_str));
        lv_roller_get_selected_str(roller_min, min_str, sizeof(min_str));
        int hour = atoi(ora_str);
        int minute = atoi(min_str);

        struct tm tm = {};
        tm.tm_year = selectedDate.year - 1900;
        tm.tm_mon = selectedDate.month - 1;
        tm.tm_mday = selectedDate.day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = 0;

        time_t newTime = mktime(&tm);
        struct timeval now = { .tv_sec = newTime };
        settimeofday(&now, nullptr);

        Serial.printf("[SetTimeScreen] ✅ Nuovo orario impostato: %s", ctime(&newTime));

        // Salva su SD
        TimeManager::saveTime();

        // Torna alla schermata principale
        backHome = true;
    }
static void calendar_event_handler(lv_event_t *e) {
    Serial.println("calendar_event_handler start");

    auto *screen = static_cast<SetTimeScreen *>(lv_event_get_user_data(e));
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        lv_calendar_date_t date;
        if(lv_calendar_get_pressed_date(obj, &date)) {
            Serial.printf("Clicked date: %02d.%02d.%d", date.day, date.month, date.year);
            screen->selectedDate = date;
        }
    }

}


};
