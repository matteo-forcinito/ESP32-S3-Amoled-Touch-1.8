#pragma once
#include "AppScreen.h"
#include "BleMouse.h"

bool connected = false;
bool touching;
bool dragging;
bool isVWheel = false;
bool isHWheel = false;
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
            isVWheel = (x > LCD_WIDTH - 40);
            isHWheel = (y > LCD_HEIGHT - 40);
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
                    if(isVWheel) {
                      bleMouse.move(0, 0, deltaY, 0);
                    } else if(isHWheel) {
                      bleMouse.move(0, 0, 0, deltaX);
                    } else {
                      bleMouse.move(deltaX, deltaY);
                    }
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
                    if(isVWheel || isHWheel) {
                      bleMouse.click(MOUSE_MIDDLE);
                    } else {
                      bleMouse.click(MOUSE_LEFT);
                    }
                }
            }

            if(bleMouse.isPressed()) bleMouse.release();

            dragging = false;
            touchedTime = 0;
        }
        
            isVWheel = false;
            isHWheel = false;
    }
}
};