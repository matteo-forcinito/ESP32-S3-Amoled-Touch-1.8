#pragma once
#include <WebServer.h>
#include "AppScreen.h"
#include "WiFiScreen.h"

class WebServerScreen : public AppScreen {
private:
  enum class Type {
      AP,
      STA,
      IDLE
  };

  enum class Status {
    IDLE,
    RUNNING,
    ERROR_WIFI_DISCONNECTED
  };

  Type type = Type::IDLE;
  Status status = Status::IDLE;
  Status lastStatus = Status::IDLE;
  String ipAddress = "";
  WebServer server{80};
  bool serverStarted = false;

  String user = "admin";
  String pwd = "admin";

public:

  void onCreate() override {
      lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_flex_align(root,
                      LV_FLEX_ALIGN_CENTER,  // orizzontalmente
                      LV_FLEX_ALIGN_CENTER,  // verticalmente tra i figli
                      LV_FLEX_ALIGN_CENTER); // spazio tra items
      lv_obj_set_style_pad_all(root, 10, 0);

      createButtons();
  }

  void createButtons() {
    lv_obj_t *button = lv_obj_create(root);
    lv_obj_set_width(button, lv_pct(100));
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_flex_flow(button, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(button,
                    LV_FLEX_ALIGN_CENTER,  // orizzontalmente
                    LV_FLEX_ALIGN_CENTER,  // verticalmente tra i figli
                    LV_FLEX_ALIGN_CENTER); // spazio tra items

    lv_obj_t *btnAP = lv_btn_create(button);
    lv_obj_add_event_cb(btnAP, [](lv_event_t *e) {
        auto self = static_cast<WebServerScreen*>(lv_event_get_user_data(e));
        self->startAP();
    }, LV_EVENT_CLICKED, this);
    lv_obj_t *lblAP = lv_label_create(btnAP);
    lv_label_set_text(lblAP, "AP");

    button = lv_obj_create(root);
    lv_obj_set_width(button, lv_pct(100));
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_flex_flow(button, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(button,
                    LV_FLEX_ALIGN_CENTER,  // orizzontalmente
                    LV_FLEX_ALIGN_CENTER,  // verticalmente tra i figli
                    LV_FLEX_ALIGN_CENTER); // spazio tra items

    lv_obj_t *btnSTA = lv_btn_create(button);
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

    if(lastStatus == status) return;

    lv_obj_clean(root);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root,
                    LV_FLEX_ALIGN_CENTER,  // orizzontalmente
                    LV_FLEX_ALIGN_CENTER,  // verticalmente tra i figli
                    LV_FLEX_ALIGN_CENTER); // spazio tra items
    lv_obj_set_style_pad_all(root, 10, 0);

    switch(status) {
      case Status::IDLE: {
        createButtons();

        break;
      }

      case Status::RUNNING: {
        lv_obj_t *lblIp = lv_label_create(root);
        lv_label_set_text(lblIp, ipAddress.c_str());

        lv_obj_t *creds = lv_obj_create(root);
        lv_obj_set_flex_flow(creds, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_height(creds, LV_SIZE_CONTENT);
        lv_obj_t *cred = lv_label_create(creds); // user
        lv_label_set_text_fmt(cred, "User: %s", user);
        cred = lv_label_create(creds); // pwd
        lv_label_set_text_fmt(cred, "Pass: %s", pwd);

        lv_obj_t *btnClose = lv_btn_create(root);
        lv_obj_add_event_cb(btnClose, [](lv_event_t *e) {
          auto self = static_cast<WebServerScreen*>(lv_event_get_user_data(e));
          self->stopServer();
        }, LV_EVENT_CLICKED, this);
        lv_obj_t *lblClose = lv_label_create(btnClose);
        lv_label_set_text(lblClose, "Close");

        break;
      }

      case Status::ERROR_WIFI_DISCONNECTED: {
        lv_obj_t *lblWifi = lv_label_create(root);
        lv_label_set_text(lblWifi, "Devi essere connesso al wifi!");
        lv_obj_t *btnWifi = lv_btn_create(root);
        lv_obj_add_event_cb(btnWifi, [](lv_event_t *e) {
          ScreenManager::get().changeScreen(new WiFiScreen());
        }, LV_EVENT_CLICKED, NULL);

        lblWifi = lv_label_create(btnWifi);
        lv_label_set_text(lblWifi, "Connect");

        break;
      }
    } 

    lastStatus = status;
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
    } else {
      status = Status::ERROR_WIFI_DISCONNECTED;
    }
  }

  void setupServer() {
    if (serverStarted) return;

    server.on("/", [this]() {
        if(!isLoggedIn()) {
           return server.requestAuthentication();
        } 
        
        String page = "<h1>Benvenuto!</h1>";
        page += "<p>Il tuo IP: " + server.client().remoteIP().toString() + "</p>";

        server.send(200, "text/html", page);
    });

    server.begin();
    serverStarted = true;

    status = Status::RUNNING;
  }

  bool isLoggedIn() {
    return server.authenticate(user.c_str(), pwd.c_str());
}

  void stopServer() {
    if(serverStarted) {
      server.close();
      serverStarted = false;
    }

    type = Type::IDLE;
    status = Status::IDLE;
  }

  void onDestroy() override {
    stopServer();
  }
};