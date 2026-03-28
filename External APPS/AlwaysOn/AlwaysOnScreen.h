#pragma once
#include "SimpleScreen.h"

extern bool enableAlwaysOn;

class AlwaysOnScreen : public SimpleScreen {
private:
  RTC_DateTime datetime = rtc.getDateTime();
  unsigned long updateTime = 0;
  int secondsUpdate = 0;
public:
  void onCreate() override {
    datetime = rtc.getDateTime();
    gfx->fillScreen(BLACK);

    // Array per mesi e giorni in italiano
    const char* mesi[] = {
      "Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno",
      "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre"
    };

    const char* giorni[] = {
      "Domenica", "Lunedi", "Martedi", "Mercoledi",
      "Giovedi", "Venerdi", "Sabato"
    };

    // Campi RTC
    int giorno = datetime.day;
    int mese = datetime.month - 1;   // ⚠️ RTC = 1-12 → array = 0-11
    int anno = datetime.year;

    // Costruisci struct tm con i dati RTC
    struct tm t;
    t.tm_year = anno - 1900;
    t.tm_mon  = mese;
    t.tm_mday = giorno;
    t.tm_hour = datetime.hour;
    t.tm_min  = datetime.minute;
    t.tm_sec  = datetime.second;

    // Calcola il weekday e timestamp locale
    time_t ts = mktime(&t);  // calcola tm_wday

    struct tm localTime;
    localtime_r(&ts, &localTime);  // applica fuso orario del sistema

    int weekday = localTime.tm_wday; // giorno della settimana corretto

    // Prima riga: Mese Anno
    gfx->setTextSize(3);
    gfx->setCursor(40, 300);
    gfx->printf("%s %d", mesi[mese], anno);

    // Seconda riga: giorno, nome giorno
    gfx->setCursor(40, 340);
    gfx->printf("%d, %s", giorno, giorni[weekday]);

    launcher();
  }

  void onTouch(int32_t x, int32_t y, int32_t fingers) {
  }

  void launcher() {
    gfx->setCursor(30, 50);
    gfx->setTextSize(2);
    gfx->print("Click boot to enter AlwayOn");

    alwaysOn();
  }

  void alwaysOn() {
    datetime = rtc.getDateTime();

    // Mostra percentuale batteria
    gfx->fillRect(100, 100, 200, 60, BLACK);
    gfx->setTextSize(5);
    gfx->setCursor(100, 100);
    gfx->println(String(power.getBatteryPercent()) + "%");


    RTC_DateTime datetime = rtc.getDateTime();
    gfx->fillRect(40, 400, 200, 60, BLACK);
    gfx->setTextSize(5);
    gfx->setCursor(40, 400);
    gfx->printf("%02d:%02d",
                datetime.hour,
                datetime.minute);

    // Se è in carica
    if (power.isCharging()) {
      gfx->fillRect(100, 200, 200, 60, BLACK);
      gfx->setTextSize(2);
      gfx->setCursor(100, 200);
      gfx->println("Charging");
    }
    
    updateTime = millis();
    secondsUpdate = 60 - datetime.second;
    if(secondsUpdate <= 0) {
      secondsUpdate = 60;
    }
  }

  void onLoop() override {
    if(millis() - updateTime > (secondsUpdate * 1000)) {
      alwaysOn();
    }
  }

  void onGesture(Gesture gesture) override {
    if(gesture == Gesture::CLICK) {
      enableAlwaysOn = true;
    }
  }
};