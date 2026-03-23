#pragma once
#include <WebServer.h>
#include "AppScreen.h"

class WebServerScreen : public AppScreen {
private:
  enum class Type {
      AP,
      STA,
      IDLE
  };

  Type type = Type::IDLE;
  String ipAddress = "";
  WebServer server{80};
  bool serverStarted = false;

public:

  void onCreate() override {
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(root, 10, 0);

    lv_obj_t *btnAP = lv_btn_create(root);
    lv_obj_add_event_cb(btnAP, [](lv_event_t *e) {
        auto self = static_cast<WebServerScreen*>(lv_event_get_user_data(e));
        self->startAP();
    }, LV_EVENT_CLICKED, this);

    lv_obj_t *lblAP = lv_label_create(btnAP);
    lv_label_set_text(lblAP, "AP");

    lv_obj_t *btnSTA = lv_btn_create(root);
    lv_obj_add_event_cb(btnSTA, [](lv_event_t *e) {
        auto self = static_cast<WebServerScreen*>(lv_event_get_user_data(e));
        self->startSTA();
    }, LV_EVENT_CLICKED, this);

    lv_obj_t *lblSTA = lv_label_create(btnSTA);
    lv_label_set_text(lblSTA, "STA");
  }

  void loop() override {
    if (type != Type::IDLE) {
        server.handleClient();
    }
  }

  void startAP() {
    type = Type::AP;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-setup", "12345678");

    ipAddress = WiFi.softAPIP().toString();
    setupServer();
  }

  void startSTA() {
    if (WiFi.status() == WL_CONNECTED) {
        type = Type::STA;
        ipAddress = WiFi.localIP().toString();
        setupServer();
    }
  }

  void setupServer() {
    if (serverStarted) return;

    server.on("/", [this]() {
        server.send(200, "text/html", "<h1>Welcome " + ipAddress + "</h1>");
    });

    server.begin();
    serverStarted = true;

    lv_obj_t *lblIp = lv_label_create(root);
    lv_label_set_text(lblIp, ipAddress.c_str());
  }

  void onDestroy() override {
    if(serverStarted) {
      server.close();
    }

    type = Type::IDLE;
  }
};