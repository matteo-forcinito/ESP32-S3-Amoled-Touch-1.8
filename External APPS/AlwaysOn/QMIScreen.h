#pragma once
#include "SimpleScreen.h"
#include "SensorQMIManager.h"

class QMIScreen : public SimpleScreen {
private:
  SensorQMIManager &imu = SensorQMIManager::getInstance();
  unsigned long updateTime = 0;
  bool ready = false;

  float smoothX = 0;
  float smoothY = 0;

  int lastX = -1;
  int lastY = -1;

  const int centerX = 184; // metà 368
  const int centerY = 224; // metà 448
  const int scale = 120;   // sensibilità

  const int radius = 6;
  float offsetX = 0;
  float offsetY = 0;
public:
  void onCreate() override {
    gfx->setCursor(20, 100);
    gfx->setTextSize(2);
    gfx->print("enabling qmi..");
    ready = imu.begin();
    if(ready) {
      imu.enable();
      
      gfx->fillRect(20, 100, 200, 50, BLACK);
      drawLines();
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
    if(imu.isEnabled()) {
      if(g == Gesture::UP || g == Gesture::DOWN) {
        ready = false;
        gfx->fillScreen(BLACK);
        gfx->setTextSize(2);
        gfx->print("calibrating.. keep device still");
        delay(2500);
        //imu.calibrateGyro();
        IMUdata acc = imu.getAccel();
        offsetX = acc.x;
        offsetY = acc.y;
        gfx->fillScreen(BLACK);
        
        drawLines();

        ready = true;
      } else {
        ready = false;
        gfx->fillScreen(BLACK);
        gfx->setTextSize(2);
        gfx->print("resetting.. keep device still");
        delay(2500);
        //imu.calibrateGyro();
        offsetX = 0;
        offsetY = 0;
        gfx->fillScreen(BLACK);
        
        drawLines();

        ready = true;
      }
    }
  }

  void onLoop() {
      if (!ready) return;

      if (imu.update()) {
          IMUdata acc = imu.getAccel();

          // 🔹 smoothing con assi corretti
          smoothX = smoothX * 0.9f + (acc.y - offsetY) * 0.1f;
          smoothY = smoothY * 0.9f + (-(acc.x - offsetX)) * 0.1f;

          // 🔹 conversione posizione
          int x = centerX + smoothX * scale;
          int y = centerY + smoothY * scale;

          // 🔹 clamp (non uscire dallo schermo)
          x = constrain(x, 0, 368);
          y = constrain(y, 0, 448);
          if (lastX != -1) {
              // 🔹 cancella pallino vecchio
              gfx->fillCircle(lastX, lastY, radius, BLACK);

              // 🔹 ridisegna elementi statici sopra quell’area
              redrawStaticAt(lastX, lastY);
          }
          // (croce o cerchi)
          int dx = lastX - centerX;
          int dy = lastY - centerY;

          float dist = sqrt(dx*dx + dy*dy);

          // se era vicino alla croce → ridisegna croce localmente
          if (abs(dy) < 2) {
              gfx->drawLine(lastX - radius, centerY, lastX + radius, centerY, DARKGREY);
          }
          if (abs(dx) < 2) {
              gfx->drawLine(centerX, lastY - radius, centerX, lastY + radius, DARKGREY);
          }

          // se era sul cerchio centrale
          if (dist < 25 && dist > 15) {
              gfx->drawCircle(centerX, centerY, 10, GREEN);
          }

          // 🔹 disegna nuovo pallino
          gfx->fillCircle(x, y, radius, GREEN);

          lastX = x;
          lastY = y;

          // 🔹 aggiorna solo area testo (no full refresh)
          gfx->fillRect(0, 0, 180, 60, BLACK);

          gfx->setCursor(0, 10);
          gfx->setTextSize(2);
          gfx->printf("X: %.2f\n", acc.x);
          gfx->printf("Y: %.2f\n", acc.y);
          gfx->printf("Z: %.2f\n", acc.z);
      }
  }

  void drawLines() {
    // 🔹 croce
    gfx->drawLine(centerX - 120, centerY, centerX + 120, centerY, DARKGREY);
    gfx->drawLine(centerX, centerY - 120, centerX, centerY + 120, DARKGREY);

    // 🔹 cerchio esterno (range)
    gfx->drawCircle(centerX, centerY, 120, DARKGREY);

    // 🔹 cerchio centrale (zona "ok")
    gfx->drawCircle(centerX, centerY, 10, GREEN);
  }

  void redrawStaticAt(int x, int y) {
    int dx = x - centerX;
    int dy = y - centerY;
    float dist = sqrt(dx*dx + dy*dy);

    // 🔹 croce (usa tolleranza = raggio pallino)
    if (abs(dy) < radius + 1) {
        gfx->drawLine(x - radius, centerY, x + radius, centerY, DARKGREY);
    }
    if (abs(dx) < radius + 1) {
        gfx->drawLine(centerX, y - radius, centerX, y + radius, DARKGREY);
    }

    // 🔹 cerchio centrale
    if (dist >= 10 - radius && dist <= 10 + radius) {
        gfx->drawCircle(centerX, centerY, 10, GREEN);
    }

    // 🔹 cerchio esterno (QUESTO ti mancava!)
    if (dist >= 120 - radius && dist <= 120 + radius) {
        gfx->drawCircle(centerX, centerY, 120, DARKGREY);
    }
  }

  void onDestroy() override {
    imu.disable();
  }
};