#pragma once
#include "SimpleScreen.h"

class FlashScreen : public SimpleScreen {
public:
  void onCreate() override {
    gfx->fillScreen(WHITE);
  }
};