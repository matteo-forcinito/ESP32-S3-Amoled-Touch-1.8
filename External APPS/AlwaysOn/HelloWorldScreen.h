#pragma once
#include "SimpleScreen.h"

class HelloWorldScreen : public SimpleScreen {
public:
  void onCreate() override {
    gfx->fillScreen(BLACK);
    gfx->setCursor(50, 300);
    gfx->setTextSize(4);
    gfx->print("Hello, World!");
  }

  void onGesture(Gesture gesture) override {
    gfx->fillRect(200, 80, 200, 60, BLACK);
    gfx->setCursor(200, 80);
    gfx->setTextSize(2);
    String action = ""; 
    switch(gesture) {
      case Gesture::CLICK: {
        action = "click";
        break;
      }

      case Gesture::RIGHT: {
        action = "right";
        break;
      }

      case Gesture::LEFT: {
        action = "left";
        break;
      }

      case Gesture::DOWN: {
        action = "down";
        break;
      }

      case Gesture::UP: {
        action = "up";
        break;
      }
    }
    gfx->print(action);
  }
};