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

  String cSSID = "SmartBox";
  String cPWD = "12345678";
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
    lv_obj_remove_style_all(button);
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
    lv_label_set_text(lblAP, "Access Point");

    button = lv_obj_create(root);
    lv_obj_remove_style_all(button);
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
    lv_label_set_text(lblSTA, "My Network");
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

        lv_obj_t *creds;
        lv_obj_t *cred;
        if(type == Type::AP) {
          creds = lv_obj_create(root);
          lv_obj_set_flex_flow(creds, LV_FLEX_FLOW_COLUMN);
          lv_obj_set_size(creds, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
          cred = lv_label_create(creds); // user
          lv_label_set_text_fmt(cred, "WiFi: %s", cSSID);
          cred = lv_label_create(creds); // pwd
          lv_label_set_text_fmt(cred, "Pass: %s", cPWD);

        }

        creds = lv_obj_create(root);
        lv_obj_set_flex_flow(creds, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_size(creds, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        cred = lv_label_create(creds); // user
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
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(cSSID.c_str(), cPWD.c_str());

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
        page += "<a href='/addNetwork'> Configure WiFi </a>";

        server.send(200, "text/html", page);
    });

    server.on("/addNetwork", [this]() {
      if (!isLoggedIn()) return server.requestAuthentication();

      // scan in background
      int n = WiFi.scanNetworks();
      String page = "<h1>Seleziona rete</h1>";
      page += "<form method='POST' action='/saveNetwork'>";
      page += "<select name='ssid'>";
      for (int i = 0; i < n; ++i) {
          page += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
      }
      page += "</select>";
      page += "<input type='password' name='pwd'>";
      page += "<button type='submit'>Salva</button>";
      page += "</form>";
      server.send(200, "text/html", page);
  });

  // POST: salva
  server.on("/saveNetwork", HTTP_POST, [this]() {
      if (!isLoggedIn()) return server.requestAuthentication();

      String ssid = server.arg("ssid");
      String pwd  = server.arg("pwd");

      // Test di connessione veloce
      WiFi.begin(ssid.c_str(), pwd.c_str());
      unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
          delay(100);
      }

      if (WiFi.status() == WL_CONNECTED) {
          WifiManager wm;
          if(wm.saveNetwork("/networks.json", ssid, pwd)) {
            server.send(200, "text/html", "<h1>Salvata!</h1>");
          } else {
            server.send(200, "text/html", "<h1>Errore nel salvataggio!</h1>");
          }
      } else {
          server.send(200, "text/html", "<h1>Errore di connessione!</h1>");
      }
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