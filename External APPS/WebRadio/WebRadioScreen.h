#include "AppScreen.h"
#include "WiFiScreen.h"
#include <Audio.h>

Audio audio;

class WebRadioScreen : public AppScreen {
public:
  void onCreate() override {
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(root, 10, 0);

    if(WiFi.status() != WL_CONNECTED) {
      lv_obj_set_flex_align(root,
                      LV_FLEX_ALIGN_CENTER,  // orizzontalmente
                      LV_FLEX_ALIGN_CENTER,  // verticalmente tra i figli
                      LV_FLEX_ALIGN_CENTER); // spazio tra items
      lv_obj_t *fbContainer = lv_obj_create(root);
      lv_obj_set_flex_flow(fbContainer, LV_FLEX_FLOW_COLUMN);
      lv_obj_t *fbLbl = lv_label_create(fbContainer);
      lv_label_set_text(fbLbl, "Devi essere connesso al WiFi!");
      lv_obj_t *fbBtn = lv_btn_create(fbContainer);
      lv_obj_add_event_cb(fbBtn, [](lv_event_t *e) {
        ScreenManager::get().changeScreen(new WiFiScreen());
      }, LV_EVENT_CLICKED, NULL);
      fbLbl = lv_label_create(fbLbl);
      lv_label_set_text(fbLbl, "Connetti");

      return;
    }
    audio.setPinout(26, 25, 22);
    audio.connecttohost("stream.srg-ssr.ch", "/m/rsj/mp3_128");
  }
};