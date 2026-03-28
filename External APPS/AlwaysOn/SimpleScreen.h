#pragma once

extern Arduino_GFX *gfx;
extern SensorPCF85063 rtc;
extern XPowersPMU power;

class SimpleScreen {
private:
  bool wasTouching = false;
  int32_t startX; int32_t startY;
public:

  enum class Gesture {
    IDLE,
    CLICK,
    LEFT,
    RIGHT,
    UP,
    DOWN
  };
  SimpleScreen() {}
  virtual ~SimpleScreen() {}

  virtual void onCreate() = 0;
  virtual void create() { gfx->fillScreen(BLACK); onCreate(); }
  virtual void onLoop() {}
  virtual void loop() { onLoop(); }
  virtual void touch(int32_t x, int32_t y, int32_t fingers) {
    onTouch(x, y, fingers);
    if(fingers > 0) {
      if(!wasTouching) {
        startX = x;
        startY = y;
        wasTouching = true;
      }
      return;
    }
    if(!wasTouching) return;
    wasTouching = false;

    int32_t absX = abs(startX - x);
    int32_t absY = abs(startY - y);
    Gesture gesture = Gesture::IDLE;
    if(absX < 50 && absY < 50) {
      if(x > 80 && x < 200 && y > 80 && y < 200) gesture = Gesture::CLICK;
    } else if(absX > absY) { // orizzontale
      if(startX < x) gesture = Gesture::RIGHT;
      else gesture = Gesture::LEFT;
    } else { // verticale
      if(startY < y) gesture = Gesture::DOWN;
      else gesture = Gesture::UP;
    }
    onGesture(gesture);
  }
  virtual void onTouch(int32_t x, int32_t y, int32_t fingers) {}
  virtual void onGesture(Gesture gesture) {}
};