#pragma once
#include <lvgl.h>
#include "BleManager.h"

class BaseScreen {
public:
  BaseScreen(BleManager *b) : ble(b), scr(nullptr) {}
  virtual ~BaseScreen() {}
  virtual void enter() = 0;
  virtual void loop() = 0;
  virtual void exit() = 0;

protected:
  BleManager *ble;
  lv_obj_t *scr;

  void createHeader(const char *mode);
};
