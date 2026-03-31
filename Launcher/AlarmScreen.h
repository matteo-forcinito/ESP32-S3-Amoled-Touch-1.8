#pragma once
#include "AppScreenLayout.h"
#include "AlarmManager.h"
#include "EditAlarmScreen.h"

class AlarmScreen : public AppScreenLayout {
private:
    lv_obj_t* list = nullptr;

public:
    AlarmScreen() : AppScreenLayout("Alarms") {}

    void onCreate() override {
        AlarmManager::load();
        list = lv_obj_create(container);
        lv_obj_remove_style_all(list);
        lv_obj_set_size(list, lv_pct(100), lv_pct(100));
        lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(list, 10, 0);
        lv_obj_set_style_pad_gap(list, 10, 0);

        renderList();

        lv_obj_t* btn = lv_btn_create(container);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, "+ Add Alarm");
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            // TODO: aprire editor sveglia
        }, LV_EVENT_CLICKED, NULL);
    }

    void renderList() {
        lv_obj_clean(list);
        const auto& alarms = AlarmManager::getAll();

        for (size_t i = 0; i < AlarmManager::getSystemAlarmCount(); i++) {
            createAlarmItemSystem(AlarmManager::getSystemAlarm(i));
        }

        if (alarms.empty()) {
            lv_obj_t* empty = lv_label_create(list);
            lv_label_set_text(empty, "No alarms");
            return;
        }

        for (size_t i = 0; i < alarms.size(); i++) {
            createAlarmItem(i, alarms[i]);
        }
    }

    void createAlarmItem(size_t index, const Alarm& alarm) {
        lv_obj_t* row = lv_obj_create(list);
        lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_all(row, 10, 0);
        lv_obj_set_style_pad_gap(row, 10, 0);

        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, [](lv_event_t *e) {
            uint32_t id = (uint32_t)lv_event_get_user_data(e);

            Alarm* alarm = AlarmManager::getById(id);
            if (!alarm) return;

            ScreenManager::get().openModal(new EditAlarmScreen(alarm->id));
        }, LV_EVENT_CLICKED, (void*)alarm.id);

        lv_obj_t* col = lv_obj_create(row);
        lv_obj_set_height(col, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 0, 0);
        lv_obj_set_flex_grow(col, 1);

        lv_obj_t* timeLabel = lv_label_create(col);
        char timeStr[6];
        sprintf(timeStr, "%02d:%02d", alarm.hour, alarm.minute);
        lv_label_set_text(timeLabel, timeStr);
        lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_20, 0);

        // Verifica se la sveglia è imminente (5 minuti)
        if (alarm.enabled && AlarmManager::isAlarmImminent(alarm, 5)) {
            lv_obj_set_style_text_color(timeLabel, lv_color_hex(0xFF0000), 0); // rosso
        }

        lv_obj_t* desc = lv_label_create(col);
        String text = alarm.title.length() ? alarm.title : "Alarm";
        lv_label_set_text(desc, text.c_str());

        lv_obj_t* sw = lv_switch_create(row);
        lv_obj_add_state(sw, alarm.enabled ? LV_STATE_CHECKED : 0);
        lv_obj_add_event_cb(sw, [](lv_event_t* e) {
            lv_obj_t* sw = lv_event_get_target(e);
            size_t idx = (size_t)lv_event_get_user_data(e);
            AlarmManager::toggleById(idx);
        }, LV_EVENT_VALUE_CHANGED, (void*)index);

        lv_obj_add_flag(col, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_clear_flag(timeLabel, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(desc, LV_OBJ_FLAG_CLICKABLE);
    }

    void createAlarmItemSystem(const Alarm& alarm) {
        lv_obj_t* row = lv_obj_create(list);
        lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_all(row, 10, 0);
        lv_obj_set_style_pad_gap(row, 10, 0);

        lv_obj_t* col = lv_obj_create(row);
        lv_obj_set_height(col, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 0, 0);
        lv_obj_set_flex_grow(col, 1);

        lv_obj_t* timeLabel = lv_label_create(col);
        char timeStr[6];
        sprintf(timeStr, "%02d:%02d", alarm.hour, alarm.minute);
        lv_label_set_text(timeLabel, timeStr);
        lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_20, 0);

        // Verifica se la sveglia è imminente (5 minuti)
        if (alarm.enabled && AlarmManager::isAlarmImminent(alarm, 5)) {
            lv_obj_set_style_text_color(timeLabel, lv_color_hex(0xFF0000), 0); // rosso
        }

        lv_obj_t* desc = lv_label_create(col);
        String text = alarm.title.length() ? alarm.title : "Alarm";
        lv_label_set_text(desc, text.c_str());

        lv_obj_t* sw = lv_switch_create(row);
        lv_obj_add_state(sw, alarm.enabled ? LV_STATE_CHECKED : 0);
        lv_obj_add_event_cb(sw, [](lv_event_t* e) {
            lv_obj_t* sw = lv_event_get_target(e);
            size_t idx = (size_t)lv_event_get_user_data(e);
            AlarmManager::toggleById(idx);
        }, LV_EVENT_VALUE_CHANGED, (void*)index);

        lv_obj_add_flag(col, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_clear_flag(timeLabel, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(desc, LV_OBJ_FLAG_CLICKABLE);
    }

    void onLoop() override {
        // aggiorna ogni minuto per evidenziare sveglie imminenti
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 60000) {
            renderList();
            lastUpdate = millis();
        }
    }
};