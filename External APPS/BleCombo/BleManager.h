#pragma once
#include <Arduino.h>
#include <BleCombo.h>  // Libreria che include Keyboard + Mouse BLE

class BleManager {
public:
  BleManager();

  void begin();
  bool isConnected();

  // Keyboard API
  void sendKey(char c);
  void sendSpecialKey(uint8_t keycode);
  void sendText(const String &txt);

  // Mouse API
  void moveMouse(int dx, int dy);
  void clickMouse(uint8_t btn = MOUSE_LEFT);
  void pressMouse(uint8_t btn = MOUSE_LEFT);
  void releaseMouse(uint8_t btn = MOUSE_LEFT);
  bool isMousePressed();

private:
  bool connected;
};
