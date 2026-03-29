#pragma once
#include "BaseScreen.h"
#include <vector>

class KeyboardScreen : public BaseScreen {
public:
  KeyboardScreen(BleManager *b);
  void enter() override;
  void onLoop() override;
  void exit() override;

private:
  bool shift = false;
  std::vector<lv_obj_t*> keys;
  void createKeyRow(lv_obj_t *parent, const char *keys[], int count);
  void onKeyPressed(const char *key);
};
