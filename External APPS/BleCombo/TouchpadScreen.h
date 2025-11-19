#pragma once
#include "BaseScreen.h"
#include "BleManager.h"
#include <lvgl.h>

class TouchpadScreen : public BaseScreen {
public:
  TouchpadScreen(BleManager *b);
  void enter() override;
  void loop() override;
  void exit() override;

private:
  static void lv_pointer_event_cb(lv_event_t *e);
};
