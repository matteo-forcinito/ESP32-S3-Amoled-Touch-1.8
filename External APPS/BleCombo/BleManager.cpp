#include "BleManager.h"

BleManager::BleManager() : connected(false) {}

void BleManager::begin() {
  Keyboard.begin();
  Mouse.begin();
}

bool BleManager::isConnected() {
  connected = Keyboard.isConnected();
  return connected;
}

// ---------- Keyboard ----------
void BleManager::sendKey(char c) {
  if (!isConnected()) return;
  Keyboard.write(c);
}

void BleManager::sendSpecialKey(uint8_t keycode) {
  if (!isConnected()) return;
  Keyboard.press(keycode);
  delay(20);
  Keyboard.release(keycode);
}

void BleManager::sendText(const String &txt) {
  if (!isConnected()) return;
  Keyboard.print(txt);
}

// ---------- Mouse ----------
void BleManager::moveMouse(int dx, int dy) {
  if (!isConnected()) return;
  Mouse.move(dx, dy);
}

void BleManager::clickMouse(uint8_t btn) {
  if (!isConnected()) return;
  Mouse.click(btn);
}

void BleManager::pressMouse(uint8_t btn) {
  if (!isConnected()) return;
  Mouse.press(btn);
}

void BleManager::releaseMouse(uint8_t btn) {
  if (!isConnected()) return;
  Mouse.release(btn);
}

bool BleManager::isMousePressed() {
  // Nota: la libreria BleCombo non espone direttamente lo stato,
  // ma possiamo mantenerlo con una variabile locale se serve in futuro.
  // Per ora ritorna sempre false per compatibilità.
  return false;
}
