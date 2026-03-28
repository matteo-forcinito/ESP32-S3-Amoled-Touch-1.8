#pragma once
#include "SimpleScreen.h"

extern bool backToLaucher;

class SettingsScreen : public SimpleScreen {
private:
  int selectedIndex = -1;
  int items = 2;
  bool backConfirmed = false;
public:
  void onCreate() override {
    draw();
  }

  void onGesture(Gesture gesture) override {
    switch(gesture) {
      case Gesture::UP: {
        if(selectedIndex <= 0) {
          selectedIndex = items;
        } else {
          selectedIndex--;
        }
        draw();

        break;
      }
      case Gesture::DOWN: {
        if(selectedIndex >= items) {
          selectedIndex = 0;
        } else {
          selectedIndex++;
        }
        draw();

        break;
      }

      case Gesture::CLICK: {
        if(selectedIndex == 2) {
          selectedIndex = -2;
        } else if(selectedIndex == -2) {
          backToLaucher = true;
        }

        draw();

        break;
      }

      case Gesture::IDLE: {
        break;
      }

      default: {
        draw();

        break;
      }
    }
  }

  void draw() {
    gfx->fillScreen(BLACK);
    switch(selectedIndex) {
      case 0: {
        gfx->setCursor(20, 100);
        gfx->setTextSize(4);
        gfx->print("Boot Time");
        gfx->setCursor(20, 250);
        gfx->setTextSize(3);
        uint64_t s = esp_timer_get_time() / 1000000ULL;

        uint32_t hours   = s / 3600;
        s %= 3600;
        uint32_t minutes = s / 60;
        uint32_t seconds = s % 60;

        // Prepara la stringa
        char buf[16];
        snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hours, minutes, seconds);
        gfx->print(buf);

        break;
      }
      case 1: {
        gfx->setCursor(20, 100);
        gfx->setTextSize(4);
        gfx->print("Test");
        break;
      }
      case 2: {
        gfx->setCursor(20, 100);
        gfx->setTextSize(4);
        gfx->print("Back to launcher");
        break;
      }
      case -1: {
        gfx->setCursor(20, 180);
        gfx->setTextSize(4);
        gfx->print("Settings");
        break;
      }
      case -2: {
        gfx->setCursor(20, 180);
        gfx->setTextSize(2);
        gfx->print("Confirm to go back launcher?");
        gfx->setCursor(20, 280);
        gfx->setTextSize(2);
        gfx->print("Click to confirm, swipe to cancel");

        break;
      }
    }
  }
};