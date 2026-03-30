#pragma once
#include "AppScreenLayout.h"
#include "HWCDC.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "WiFiScreen.h"

extern HWCDC USBSerial;

class MeteoScreen : public AppScreenLayout {
private:
  bool ready = false;

  String cityCode = "Civita Castellana";
  const char* api_key = "fbf5a0e942e6fea3ff18103b9fd46ed9";

  HTTPClient httpClient;

  // UI
  lv_obj_t *tempBig = nullptr;
  lv_obj_t *tempMinMax = nullptr;
  lv_obj_t *iconImg = nullptr;
  lv_obj_t *descLabel = nullptr;

  lv_obj_t *hum = nullptr;
  lv_obj_t *press = nullptr;
  lv_obj_t *vis = nullptr;

  unsigned long lastUpdate = 0;

public:
  MeteoScreen() : AppScreenLayout("Meteo") {}

  // ---------------- UI HELPERS ----------------

  lv_obj_t* createRow(const char* title) {
    lv_obj_t *row = lv_obj_create(container);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(row, 10, 0);
    lv_obj_set_style_pad_gap(row, 10, 0);

    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, title);

    lv_obj_t *value = lv_label_create(row);
    return value;
  }

  // Placeholder icone (da sostituire)
  const char* getWeatherIconPath(const char* icon) {
      if (!icon || strlen(icon) < 3) {
          return "S:/assets/icons/weather-cloud.bin";
      }

      bool isDay = icon[2] == 'd';

      switch (icon[0]) {

          case '0': // 01–09
              switch (icon[1]) {
                  case '1': // clear sky
                      return isDay
                          ? "S:/assets/icons/weather-sun.bin"
                          : "S:/assets/icons/weather-moon.bin";

                  case '2': // few clouds
                      return isDay
                          ? "S:/assets/icons/weather-cloud-sun.bin"
                          : "S:/assets/icons/weather-cloud-moon.bin";

                  case '3': // scattered clouds
                  case '4': // broken clouds
                      return "S:/assets/icons/weather-cloud.bin";

                  case '9': // shower rain
                      return "S:/assets/icons/weather-rain.bin";
              }
              break;

          case '1': // 10–19
              switch (icon[1]) {
                  case '0': // rain
                      return "S:/assets/icons/weather-rain.bin";

                  case '1': // thunderstorm
                      return "S:/assets/icons/weather-storm.bin";

                  case '3': // snow
                      return "S:/assets/icons/weather-snow.bin";
              }
              break;

          case '5': // 50 = mist
              if (icon[1] == '0') {
                  return "S:/assets/icons/weather-fog.bin";
              }
              break;
      }

      return "S:/assets/icons/weather-cloud.bin"; // fallback
  }

  // ---------------- CREATE ----------------

  void onCreate() override {
    if (WiFi.status() != WL_CONNECTED) {
      lv_obj_t *label = lv_label_create(container);
      lv_label_set_text(label, "Devi essere connesso al wifi!");

      lv_obj_t *btn = lv_btn_create(container);
      lv_obj_t *txt = lv_label_create(btn);
      lv_label_set_text(txt, "Connect");

      lv_obj_add_event_cb(btn, [](lv_event_t *e) {
        ScreenManager::get().openModal(new WiFiScreen());
      }, LV_EVENT_CLICKED, NULL);
      return;
    }

    ready = true;

    // -------- HERO --------
    lv_obj_t *hero = lv_obj_create(container);
    lv_obj_set_size(hero, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(hero, 10, 0);
    lv_obj_set_style_pad_gap(hero, 10, 0);

    iconImg = lv_img_create(hero);
    lv_obj_set_size(iconImg, 64, 64);

    lv_obj_t *tempCol = lv_obj_create(hero);
    lv_obj_set_flex_flow(tempCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(tempCol, 0, 0);
    lv_obj_set_style_pad_all(tempCol, 0, 0);

    tempBig = lv_label_create(tempCol);
    lv_obj_set_style_text_font(tempBig, &lv_font_montserrat_28, 0);

    tempMinMax = lv_label_create(tempCol);
    lv_obj_set_style_text_font(tempMinMax, &lv_font_montserrat_14, 0);

    descLabel = lv_label_create(container);
    lv_obj_set_style_text_align(descLabel, LV_TEXT_ALIGN_CENTER, 0);

    // -------- DETAILS --------
    hum = createRow("Humidity:");
    press = createRow("Pressure:");
    vis = createRow("Visibility:");

    update();
  }

  // ---------------- UPDATE ----------------

  void update() {
    if (!ready) return;

    String city = cityCode;
    city.replace(" ", "%20");

    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city +
                 "&appid=" + String(api_key) + "&units=metric";

    httpClient.begin(url);

    int httpCode = httpClient.GET();
    USBSerial.print("HTTP: ");
    USBSerial.println(httpCode);

    if (httpCode != HTTP_CODE_OK) {
      USBSerial.println("API error");
      USBSerial.println(httpClient.getString());
      httpClient.end();
      return;
    }

    String payload = httpClient.getString();
    httpClient.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
      USBSerial.println("JSON parse error");
      return;
    }

    if (doc["main"].isNull()) {
      USBSerial.println("Invalid JSON structure");
      return;
    }

    float temp = doc["main"]["temp"];
    float temp_min = doc["main"]["temp_min"];
    float temp_max = doc["main"]["temp_max"];
    int humidity = doc["main"]["humidity"];
    int pressure = doc["main"]["pressure"];
    int visibility = doc["visibility"];

    String description = doc["weather"][0]["description"];
    String icon = doc["weather"][0]["icon"];

    // -------- UI UPDATE --------

    lv_label_set_text(tempBig, (String(temp, 1) + "°C").c_str());

    lv_label_set_text(tempMinMax,
      ("min " + String(temp_min, 0) + "° / max " + String(temp_max, 0) + "°").c_str()
    );

    lv_label_set_text(descLabel, description.c_str());

    lv_label_set_text(hum, (String(humidity) + "%").c_str());
    lv_label_set_text(press, (String(pressure) + " hPa").c_str());
    lv_label_set_text(vis, (String(visibility / 1000.0, 1) + " km").c_str());

    const void* iconSrc = getWeatherIconPath(icon.c_str());
    if (iconSrc != NULL) {
      lv_img_set_src(iconImg, iconSrc);
    }

    lastUpdate = millis();
  }

  // ---------------- LOOP ----------------

  void onLoop() override {
    if (lastUpdate == 0 || millis() - lastUpdate > 300000) { // 5 min
      update();
    }
  }
};