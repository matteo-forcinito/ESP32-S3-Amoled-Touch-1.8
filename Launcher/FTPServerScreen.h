#pragma once
#include "AppScreenLayout.h"
#include <SimpleFtpServer.h>
#include "WiFiScreen.h"

class FTPServerScreen : public AppScreenLayout {
private:
    bool ready = false;
public:
  FTPServerScreen() : AppScreenLayout("FTP Server") {}

  FtpServer ftpSrv;

  void onCreate() override {
    if(WiFi.status() == WL_CONNECTED) {
        String ip = WiFi.localIP().toString();
        lv_obj_t *ipLabel = lv_label_create(container);
        lv_label_set_text(ipLabel, ("IP: " + ip).c_str());

        ftpSrv.begin("esp32", "12345");

        ready = true;
    } else {
        lv_obj_t *connect = lv_label_create(container);
        lv_label_set_text(connect, "Devi essere connesso al wifi!");

        lv_obj_t *btnConnect = lv_btn_create(container);
        connect = lv_label_create(btnConnect);
        lv_label_set_text(connect, "connect");
        lv_obj_add_event_cb(btnConnect, [](lv_event_t *e) {
            ScreenManager::get().openModal(new WiFiScreen());
        }, LV_EVENT_CLICKED, NULL);
    }
  }

  void onLoop() override {
    if(ready) {
      ftpSrv.handleFTP();
    }
  }
}; 
