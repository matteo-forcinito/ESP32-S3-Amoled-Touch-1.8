#include "ModeManager.h"
#include "KeyboardScreen.h"
#include "TouchpadScreen.h"
#include <Arduino.h>

ModeManager::ModeManager(BleManager *bleMgr) {
  ble = bleMgr;
  kbd = new KeyboardScreen(ble);
  mouse = new TouchpadScreen(ble);
}

ModeManager::~ModeManager() {
  if (kbd) delete kbd;
  if (mouse) delete mouse;
}

void ModeManager::begin() {
  switchTo(MODE_TYPE_KEYBOARD);
}

void ModeManager::switchTo(ModeType t) {
  if (active) active->exit();
  activeType = t;
  if (t == MODE_TYPE_KEYBOARD) active = kbd;
  else active = mouse;
  if (active) active->enter();
}

void ModeManager::toggleMode() {
  if (!ble->isConnected()) {
    Serial.println("Cannot toggle mode: BLE not connected");
    return;
  }
  if (activeType == MODE_TYPE_KEYBOARD) switchTo(MODE_TYPE_MOUSE);
  else switchTo(MODE_TYPE_KEYBOARD);
}

void ModeManager::loop() {
  if (active) active->loop();
}

ModeType ModeManager::current() { return activeType; }
