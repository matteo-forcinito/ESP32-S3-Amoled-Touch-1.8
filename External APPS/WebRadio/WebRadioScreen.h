#include "AppScreen.h"
#include "WiFiScreen.h"
#include <WiFi.h>

#include "AudioFileSourceICYStream.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// === AUDIO GLOBAL (come facevi tu con Audio audio;) ===
static AudioGeneratorMP3 *mp3 = nullptr;
static AudioFileSourceICYStream *file = nullptr;
static AudioOutputI2S *out = nullptr;

class WebRadioScreen : public AppScreen {
public:

  void onCreate() override {
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(root, 10, 0);

    // ---- CHECK WIFI ----
    if (WiFi.status() != WL_CONNECTED) {
      lv_obj_set_flex_align(root,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

      lv_obj_t *fbContainer = lv_obj_create(root);
      lv_obj_set_flex_flow(fbContainer, LV_FLEX_FLOW_COLUMN);

      lv_obj_t *fbLbl = lv_label_create(fbContainer);
      lv_label_set_text(fbLbl, "Devi essere connesso al WiFi!");

      lv_obj_t *fbBtn = lv_btn_create(fbContainer);
      lv_obj_add_event_cb(fbBtn, [](lv_event_t *e) {
        ScreenManager::get().changeScreen(new WiFiScreen());
      }, LV_EVENT_CLICKED, NULL);

      lv_obj_t *btnLbl = lv_label_create(fbBtn);
      lv_label_set_text(btnLbl, "Connetti");

      return;
    }

    // ---- AMP ON ----
    pinMode(PA, OUTPUT);
    digitalWrite(PA, HIGH);

    // ---- AUDIO OUTPUT ----
    out = new AudioOutputI2S();
    out->SetPinout(BCLKPIN, WSPIN, DOPIN);
    out->SetGain(0.5); // volume

    // ---- STREAM RADIO (MP3) ----
    file = new AudioFileSourceICYStream("http://icecast.omroep.nl/radio1-bb-mp3");
    file->SetReconnect(5, 1000);

    mp3 = new AudioGeneratorMP3();
    mp3->begin(file, out);

    // ---- UI FEEDBACK ----
    lv_obj_t *lbl = lv_label_create(root);
    lv_label_set_text(lbl, "Riproduzione radio...");
  }

  void loop() override {
    if (mp3 && mp3->isRunning()) {
      if (!mp3->loop()) {
        mp3->stop();
      }
    }
  }

  void onDestroy() override {
    // ---- CLEANUP (IMPORTANTISSIMO) ----
    if (mp3) {
      mp3->stop();
      delete mp3;
      mp3 = nullptr;
    }

    if (file) {
      delete file;
      file = nullptr;
    }

    if (out) {
      delete out;
      out = nullptr;
    }

    // opzionale: spegni amp
    digitalWrite(PA, LOW);
  }
};