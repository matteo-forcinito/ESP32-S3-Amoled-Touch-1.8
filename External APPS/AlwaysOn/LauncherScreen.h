#pragma once
#include "SimpleScreen.h"
#include "HelloWorldScreen.h"
#include "FlashScreen.h"
#include "SettingsScreen.h"
#include "QMIScreen.h"
#include "WebServerScreen.h"

struct AppItem {
  const char* name;
  SimpleScreen* (*create)();
};

SimpleScreen* createClock() { return new AlwaysOnScreen(); }
SimpleScreen* createWebServer() { return new WebServerScreen(); }
SimpleScreen* createHelloWorld() { return new HelloWorldScreen(); }
SimpleScreen* createFlash() { return new FlashScreen(); }
SimpleScreen* createSettings() { return new SettingsScreen(); }
SimpleScreen* createQMI() { return new QMIScreen(); }

AppItem apps[] = {
  { "Clock", createClock },
  { "QMI", createQMI },
  { "WebServer", createWebServer },
  { "Hello World", createHelloWorld },
  { "Flash", createFlash },
  { "Settings", createSettings }
};

int appCount = sizeof(apps) / sizeof(AppItem);
extern SimpleScreen* openApp;

class LauncherScreen : public SimpleScreen {
private:
  int selectedIndex = 0;

public:

  void onCreate() override {
    draw();
  }

  void onLoop() override {
    // opzionale: animazioni future
  }

  void onGesture(Gesture gesture) override {
    switch (gesture) {
      case Gesture::UP:
        if (selectedIndex > 0) {
          selectedIndex--;
        } else {
          selectedIndex = appCount - 1;
        }
        draw();
        break;

      case Gesture::DOWN:
        if (selectedIndex < appCount - 1) {
          selectedIndex++;
        } else {
          selectedIndex = 0;
        }
        draw();
        break;

      case Gesture::CLICK:
        launchApp(selectedIndex);
        break;

      default:
        break;
    }
  }

private:

  void draw() {
    gfx->fillScreen(0x0000); // nero AMOLED
    RTC_DateTime datetime = rtc.getDateTime();
    gfx->setTextSize(2);
    gfx->setCursor(50, 20);
    gfx->printf("%02d:%02d",
                datetime.hour,
                datetime.minute);
    gfx->setCursor(300, 20);
    gfx->println(String(power.getBatteryPercent()) + "%");

    gfx->setTextSize(3);
    gfx->setCursor(80, 180);
    gfx->print(apps[selectedIndex].name);
  }

  void launchApp(int index) {
    if (openApp) {
      openApp->onDestroy();
      delete openApp;
      openApp = nullptr;
    }

    openApp = apps[index].create();
    openApp->create();
  }
};