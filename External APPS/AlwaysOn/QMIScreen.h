#pragma once
#include "SimpleScreen.h"
#include "SensorQMIManager.h"

class QMIScreen : public SimpleScreen {
private:
  SensorQMIManager &imu = SensorQMIManager::getInstance();
  unsigned long updateTime = 0;
  bool ready = false;
public:
  void onCreate() override {
    gfx->setCursor(20, 100);
    gfx->setTextSize(2);
    gfx->print("enabling qmi..");
    ready = imu.begin();
    if(ready) {
      imu.enable();
    } else {
      gfx->setCursor(20, 100);
      gfx->fillScreen(BLACK);
      gfx->setTextSize(2);
      gfx->print("error QMI");

      ready = false;
    }

    delay(1500);
  }

  void onGesture(Gesture g) {
    if(imu.isEnabled() && (g == Gesture::UP || g == Gesture::DOWN)) {
      ready = false;
      gfx->fillScreen(BLACK);
      gfx->setTextSize(2);
      gfx->print("calibrating.. keep device still");
      imu.calibrateGyro();
      gfx->fillScreen(BLACK);

      ready = true;
    }
  }

  void onLoop() {
    if(!ready) return; 
    if(updateTime != 0 || millis() - updateTime > 500) {
      updateTime = millis();
      gfx->fillScreen(BLACK);
      if (imu.update()) {
          IMUdata acc = imu.getAccel();
          IMUdata gyr = imu.getGyro();

          gfx->setCursor(10, 100);
          gfx->setTextSize(2);

          gfx->printf("ACC: %.2f %.2f %.2f\n", acc.x, acc.y, acc.z);
          gfx->printf("GYR: %.2f %.2f %.2f\n", gyr.x, gyr.y, gyr.z);
      } else {
          gfx->setCursor(10, 10);
          gfx->setTextSize(2);
          gfx->print("no imu update");
      }
    }
  }

  void onDestroy() override {
    imu.disable();
  }
};