#pragma once
#include "SimpleScreen.h"
#include "WebServerManager.h"

class WebServerScreen : public SimpleScreen {
private:
  WebServerManager& ws = WebServerManager::get();

  WebServerManager::Status state = WebServerManager::Status::IDLE;
public:

  void onCreate() override {
    gfx->fillScreen(BLACK);
    gfx->setCursor(50, 50);
    gfx->setTextSize(4);
    gfx->print("WebServer");

    gfx->setCursor(50, 200);
    gfx->setTextSize(3);
    gfx->print("Connecting..");

    ws.startAP("Smartbox", "12345678");
  }

  void cleanScreen() {
    gfx->fillRect(0, 100, 400, 370, BLACK);
  }

  void onLoop() override {
    ws.loop();
    WebServerManager::Status currentState = ws.getStatus();
    if(state == currentState) return;
    
    cleanScreen();

    switch(currentState) {
      case WebServerManager::Status::RUNNING: {
        gfx->setTextSize(2);
        gfx->setCursor(50, 120);
        gfx->print(ws.getIP().c_str());

        gfx->setTextSize(3);
        gfx->setCursor(50, 200);
        gfx->print("Smartbox");
        gfx->setTextSize(2);
        gfx->setCursor(50, 240);
        gfx->print("12345678");

        gfx->setTextSize(2);
        gfx->setCursor(50, 360);
        gfx->print("admin");
        gfx->setTextSize(2);
        gfx->setCursor(50, 380);
        gfx->print("admin");


        break;
      }
      case WebServerManager::Status::IDLE: {
        gfx->setTextSize(2);
        gfx->setCursor(50, 220);
        gfx->print("Click to connect");

        break;
      }
    }

    state = currentState;
  }

  void onGesture(Gesture gesture) override {
    switch(gesture) {
      case Gesture::CLICK: {
        if(state == WebServerManager::Status::IDLE) {
          ws.startAP("Smartbox", "12345678");
        }
        break;
      }

      case Gesture::RIGHT: {
        break;
      }

      case Gesture::LEFT: {
        ws.stop();
        break;
      }

      case Gesture::DOWN: {
        break;
      }

      case Gesture::UP: {
        break;
      }
    }
  }

  void onDestroy() override {
    ws.stop();
  }
};