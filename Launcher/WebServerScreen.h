#pragma once
#include "AppScreen.h"
#include "WiFiScreen.h"
#include "WebServerManager.h"

class WebServerScreen : public AppScreen {
private:
    WebServerManager::Status lastStatus = WebServerManager::Status::IDLE;

    String cSSID = "SmartBox";
    String cPWD = "12345678";
    String user = "admin";
    String pwd = "admin";

public:

    void onCreate() override {
        setupLayout();
        createButtons();
    }

    void onLoop() override {
        auto& ws = WebServerManager::get();
        ws.loop();

        auto status = ws.getStatus();
        if (status == lastStatus) return;

        rebuildUI(status);
        lastStatus = status;
    }

private:

    void setupLayout() {
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(root,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(root, 10, 0);
    }

    void rebuildUI(WebServerManager::Status status) {
        lv_obj_clean(root);
        setupLayout();

        auto& ws = WebServerManager::get();

        switch (status) {

        case WebServerManager::Status::IDLE:
            createButtons();
            break;

        case WebServerManager::Status::RUNNING: {
            lv_obj_t *lblIp = lv_label_create(root);
            lv_label_set_text(lblIp, ws.getIP().c_str());

            if (ws.getType() == WebServerManager::Type::AP) {
                lv_obj_t *creds = lv_obj_create(root);
                lv_obj_set_size(creds, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_obj_set_flex_flow(creds, LV_FLEX_FLOW_COLUMN);

                lv_obj_t *l = lv_label_create(creds);
                lv_label_set_text_fmt(l, "WiFi: %s", cSSID);

                l = lv_label_create(creds);
                lv_label_set_text_fmt(l, "Pass: %s", cPWD);
            }

            lv_obj_t *creds = lv_obj_create(root);
            lv_obj_set_size(creds, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(creds, LV_FLEX_FLOW_COLUMN);

            lv_obj_t *l = lv_label_create(creds);
            lv_label_set_text_fmt(l, "User: %s", user);

            l = lv_label_create(creds);
            lv_label_set_text_fmt(l, "Pass: %s", pwd);

            lv_obj_t *btn = lv_btn_create(root);
            lv_obj_add_event_cb(btn, [](lv_event_t *e) {
                WebServerManager::get().stop();
            }, LV_EVENT_CLICKED, NULL);

            lv_obj_t *lbl = lv_label_create(btn);
            lv_label_set_text(lbl, "Close");

            break;
        }

        case WebServerManager::Status::ERROR_WIFI_DISCONNECTED: {
            lv_obj_t *lbl = lv_label_create(root);
            lv_label_set_text(lbl, "Devi essere connesso al wifi!");

            lv_obj_t *btn = lv_btn_create(root);
            lv_obj_add_event_cb(btn, [](lv_event_t *e) {
              WebServerManager::get().stop();
              ScreenManager::get().changeScreen(new WiFiScreen());
            }, LV_EVENT_CLICKED, NULL);

            lv_obj_t *lbl2 = lv_label_create(btn);
            lv_label_set_text(lbl2, "Connect");

            break;
        }
        }
    }

    void createButtons() {
        lv_obj_t *container = lv_obj_create(root);
        lv_obj_remove_style_all(container);
        lv_obj_set_width(container, lv_pct(100));
        lv_obj_set_flex_grow(container, 1);
        lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(container,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);

        lv_obj_t *btnAP = lv_btn_create(container);
        lv_obj_add_event_cb(btnAP, [](lv_event_t *e) {
            WebServerManager::get().startAP("SmartBox", "12345678");
        }, LV_EVENT_CLICKED, NULL);

        lv_label_set_text(lv_label_create(btnAP), "Access Point");

        container = lv_obj_create(root);
        lv_obj_remove_style_all(container);
        lv_obj_set_width(container, lv_pct(100));
        lv_obj_set_flex_grow(container, 1);
        lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(container,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);

        lv_obj_t *btnSTA = lv_btn_create(container);
        lv_obj_add_event_cb(btnSTA, [](lv_event_t *e) {
            WebServerManager::get().startSTA();
        }, LV_EVENT_CLICKED, NULL);

        lv_label_set_text(lv_label_create(btnSTA), "My Network");
    }

    void onDestroy() override {
      //  WebServerManager::get().stop();
    }
};