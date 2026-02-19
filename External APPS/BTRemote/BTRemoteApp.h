#pragma once
#include "AppScreen.h"
#include "BleMouse.h"

bool connected = false;
bool touching;
bool dragging;
unsigned long touchStartTime;
unsigned long touchReleaseTime;
int startX;
int startY;
unsigned long touchX = 0;
unsigned long touchedTime = 0;
BleMouse bleMouse;

lv_obj_t *labelStatus = nullptr;

class BTRemoteApp : public AppScreen {
private:
public:
  void onCreate() {
    labelStatus = lv_label_create(root);
    lv_label_set_text(labelStatus, "In Attesa di dispositivo");
    bleMouse.begin();
  }

  void loop() {
    if(connected && !bleMouse.isConnected()) {
      connected = false;
      lv_label_set_text(labelStatus, "In Attesa di dispositivo");
    }

    if(!connected && bleMouse.isConnected()) {
      connected = true;
      lv_label_set_text(labelStatus, "Touchpad Attivo");
    }
  }

  virtual void onTouch(int32_t x, int32_t y, int32_t fingers) {
    if(fingers > 0) {
        if(!touching) {
            // Nuovo tocco
            touching = true;
            startX = x;
            startY = y;
            dragging = false;
            touchedTime = millis();
        } else {
            int deltaX = x - startX;
            int deltaY = y - startY;


            // Se supero la soglia, inizio drag
            if(!dragging && (abs(deltaX) > 5 || abs(deltaY) > 5) && millis() - touchedTime > 100) {
            Serial.printf("%d : %d", deltaX, deltaY);
                dragging = true;
                touchReleaseTime = 0;
            }

            if(dragging) {
                // Muovo il mouse solo se c'è un delta significativo
                                if(abs(deltaX) < 50 && abs(deltaY) < 50) {
                    bleMouse.move(deltaX, deltaY);
                }

                // Aggiorno le coordinate di riferimento
                startX = x;
                startY = y;

            }
        }
    } else {
        // Rilascio
        if(touching) {
            touching = false;

            if(!dragging) {
                // Click sinistro o destro
                if(millis() - touchedTime > 1000) {
                    bleMouse.click(MOUSE_RIGHT);
                } else {
                    bleMouse.click(MOUSE_LEFT);
                }
            }

            if(bleMouse.isPressed()) bleMouse.release();

            dragging = false;
            touchedTime = 0;
        }
    }
}

  void pointer_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED) {
        // Inizio touch
        touching = true;
        lv_point_t point;
        lv_indev_get_point(lv_indev_get_act(), &point);
        startX = point.x;
        startY = point.y;
        dragging = false;
    }
    else if(code == LV_EVENT_PRESSING) {
        // Touch in corso / drag
        if(touching) {
            lv_point_t point;
            lv_indev_get_point(lv_indev_get_act(), &point);
            int deltaX = point.x - startX;
            int deltaY = point.y - startY;

            if(!dragging && (abs(deltaX) > 5 || abs(deltaY) > 5)) {
                dragging = true;
                touchedTime = 0;
            }

            if(dragging) {
            
                if(touchReleaseTime > 0) {
                  if(millis() - touchReleaseTime < 200) {
                    bleMouse.press(MOUSE_LEFT);
                  }
                  touchReleaseTime = 0;
                }

                bleMouse.move(deltaX, deltaY);
                startX = point.x;
                startY = point.y;
            } else if(touchedTime == 0) {
              touchedTime = millis();
            }
        }
    }
    else if(code == LV_EVENT_RELEASED) {
        // Fine touch
        if(touching) {
            touching = false;
            if(!dragging) {
              if(touchedTime > 0 && millis() - touchedTime > 1000) {
                bleMouse.click(MOUSE_RIGHT);
              } else {
                bleMouse.click();
              }
            }
            if(bleMouse.isPressed()) {
              bleMouse.release();
            }
            dragging = false;
            touchReleaseTime = millis();
            touchedTime = 0;
        }
    }
}
};