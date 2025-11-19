#pragma once
#include "BleManager.h"
#include "BaseScreen.h"

enum ModeType { MODE_TYPE_KEYBOARD, MODE_TYPE_MOUSE };

class ModeManager {
public:
  ModeManager(BleManager *ble);
  ~ModeManager();
  void begin();
  void switchTo(ModeType t);
  void toggleMode();
  void loop();
  ModeType current();

private:
  BleManager *ble;
  BaseScreen *active = nullptr;
  BaseScreen *kbd = nullptr;
  BaseScreen *mouse = nullptr;
  ModeType activeType = MODE_TYPE_KEYBOARD;
};
