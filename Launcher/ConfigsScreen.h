#pragma once
#include "AppScreenLayout.h"
#include "ConfigManager.h"

extern ConfigManager* config;

class ConfigsScreen : public AppScreenLayout {
public:
  ConfigsScreen() : AppScreenLayout("Configurations") {}

  void onCreate() override {
    lv_obj_t *aodColor = lv_obj_create(container);
    lv_obj_set_size(aodColor, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_t *lblAodColor = lv_label_create(aodColor);
    lv_label_set_text(lblAodColor, "Always On text color");

    lv_obj_t *btnColorRed = lv_btn_create(container);
    lv_obj_t *lblColor = lv_label_create(btnColorRed);
    lv_label_set_text(lblColor, "Red");
    lv_obj_add_event_cb(btnColorRed, [](lv_event_t *e) {
      config->setUInt16("alwaysOnColor", 0xF800);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btnColorGreen = lv_btn_create(container);
    lblColor = lv_label_create(btnColorGreen);
    lv_label_set_text(lblColor, "Green");
    lv_obj_add_event_cb(btnColorGreen, [](lv_event_t *e) {
      config->setUInt16("alwaysOnColor", 0x07E0);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btnColorBlue = lv_btn_create(container);
    lblColor = lv_label_create(btnColorBlue);
    lv_label_set_text(lblColor, "Blue");
    lv_obj_add_event_cb(btnColorBlue, [](lv_event_t *e) {
      config->setUInt16("alwaysOnColor", 0x001F);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btnColorYellow = lv_btn_create(container);
    lblColor = lv_label_create(btnColorYellow);
    lv_label_set_text(lblColor, "Yellow");
    lv_obj_add_event_cb(btnColorYellow, [](lv_event_t *e) {
      config->setUInt16("alwaysOnColor", 0xFFE0);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btnColorWhite = lv_btn_create(container);
    lblColor = lv_label_create(btnColorWhite);
    lv_label_set_text(lblColor, "White");
    lv_obj_add_event_cb(btnColorWhite, [](lv_event_t *e) {
      config->setUInt16("alwaysOnColor", 0xFFFF);
    }, LV_EVENT_CLICKED, NULL);
  }
};