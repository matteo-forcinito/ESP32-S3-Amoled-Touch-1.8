#pragma once
#include "AppScreenLayout.h"

#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_flash.h"
#include "esp_psram.h"

class AboutScreen : public AppScreenLayout {
public:
  AboutScreen() : AppScreenLayout("About") {}

  void onCreate() override {

      auto createSection = [&](const char* title) {
          lv_obj_t *c = lv_obj_create(container);
          lv_obj_set_size(c, lv_pct(100), LV_SIZE_CONTENT);
          lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
          lv_obj_set_style_pad_all(c, 10, 0);

          lv_obj_t *l = lv_label_create(c);
          lv_label_set_text(l, title);

          return c;
      };

      auto addValue = [&](lv_obj_t* parent, const char* text) {
          lv_obj_t *l = lv_label_create(parent);
          lv_label_set_text(l, text);
      };

      // ⏱️ UPTIME
      {
          lv_obj_t* c = createSection("Boot Time");

          uint64_t s = esp_timer_get_time() / 1000000ULL;
          uint32_t h = s / 3600;
          s %= 3600;
          uint32_t m = s / 60;
          uint32_t sec = s % 60;

          char buf[32];
          sprintf(buf, "%02u:%02u:%02u", h, m, sec);
          addValue(c, buf);
      }

      // 🧠 RAM
      {
          lv_obj_t* c = createSection("Memory");

          uint32_t freeHeap = esp_get_free_heap_size();
          uint32_t minHeap = esp_get_minimum_free_heap_size();

          char buf[64];
          sprintf(buf, "Free: %u KB", freeHeap / 1024);
          addValue(c, buf);

          sprintf(buf, "Min: %u KB", minHeap / 1024);
          addValue(c, buf);

          #ifdef BOARD_HAS_PSRAM
          uint32_t psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
          sprintf(buf, "PSRAM: %u KB", psram / 1024);
          addValue(c, buf);
          #endif
      }

      // ⚙️ CHIP INFO
      {
          lv_obj_t* c = createSection("Chip");

          esp_chip_info_t chip_info;
          esp_chip_info(&chip_info);

          char buf[64];
          sprintf(buf, "Cores: %d", chip_info.cores);
          addValue(c, buf);

          sprintf(buf, "Revision: %d", chip_info.revision);
          addValue(c, buf);

          sprintf(buf, "CPU: %d MHz", ESP.getCpuFreqMHz());
          addValue(c, buf);
      }

      // 💾 FLASH
      {
          lv_obj_t* c = createSection("Flash");

          uint32_t flashSize = ESP.getFlashChipSize();

          char buf[64];
          sprintf(buf, "Size: %u MB", flashSize / (1024 * 1024));
          addValue(c, buf);
      }

      // 🔋 BATTERY (extra rispetto alla tua UI header)
      {
          lv_obj_t* c = createSection("Battery");

          char buf[64];
          sprintf(buf, "Voltage: %d mV", power.getBattVoltage());
          addValue(c, buf);

          sprintf(buf, "Percent: %d%%", power.getBatteryPercent());
          addValue(c, buf);
      }

      // 🌡️ TEMPERATURA (approssimata)
      {
          lv_obj_t* c = createSection("Temperature");

          float temp = temperatureRead(); // ESP32 internal sensor

          char buf[32];
          sprintf(buf, "%.1f C", temp);
          addValue(c, buf);
      }
  }
}; 
