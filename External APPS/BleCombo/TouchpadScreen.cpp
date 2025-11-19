#include "TouchpadScreen.h"
#include "BleManager.h"
#include <Arduino.h>

TouchpadScreen::TouchpadScreen(BleManager *b) : BaseScreen(b) {}

void TouchpadScreen::enter() {
  scr = lv_obj_create(NULL);
  createHeader("Mouse");
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_scr_load(scr);

  lv_obj_t *touch_area = lv_obj_create(scr);
  lv_obj_set_size(touch_area, lv_pct(100), lv_pct(100));
  lv_obj_align(touch_area, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(touch_area, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(touch_area, lv_pointer_event_cb, LV_EVENT_ALL, ble);

  lv_obj_t *label = lv_label_create(scr);
  lv_label_set_text(label, "Touchpad attivo");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
}

void TouchpadScreen::loop() {
  // Nessuna lettura diretta del touch: LVGL chiama lv_pointer_event_cb() automaticamente
}

void TouchpadScreen::exit() {
  lv_obj_clean(scr);
  lv_obj_del(scr);
}

/**
 * Evento touch gestito da LVGL, con supporto drag, click sinistro e destro.
 * Si appoggia su BleManager per muovere e cliccare il mouse BLE.
 */
void TouchpadScreen::lv_pointer_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  BleManager *ble = static_cast<BleManager *>(lv_event_get_user_data(e));

  static bool touching = false;
  static bool dragging = false;
  static unsigned long touchedTime = 0;
  static unsigned long touchReleaseTime = 0;
  static int startX = 0, startY = 0;

  if (code == LV_EVENT_PRESSED) {
    touching = true;
    lv_point_t point;
    lv_indev_get_point(lv_indev_get_act(), &point);
    startX = point.x;
    startY = point.y;
    dragging = false;
  }
  else if (code == LV_EVENT_PRESSING) {
    if (touching) {
      lv_point_t point;
      lv_indev_get_point(lv_indev_get_act(), &point);
      int deltaX = point.x - startX;
      int deltaY = point.y - startY;

      if (!dragging && (abs(deltaX) > 5 || abs(deltaY) > 5)) {
        dragging = true;
        touchedTime = 0;
      }

      if (dragging) {
        if (touchReleaseTime > 0 && millis() - touchReleaseTime < 200) {
          ble->pressMouse(MOUSE_LEFT);
        }
        touchReleaseTime = 0;
        ble->moveMouse(deltaX, deltaY);
        startX = point.x;
        startY = point.y;
      } else if (touchedTime == 0) {
        touchedTime = millis();
      }
    }
  }
  else if (code == LV_EVENT_RELEASED) {
    if (touching) {
      touching = false;
      if (!dragging) {
        if (touchedTime > 0 && millis() - touchedTime > 1000) {
          ble->clickMouse(MOUSE_RIGHT);
        } else {
          ble->clickMouse(MOUSE_LEFT);
        }
      }
      ble->releaseMouse(MOUSE_LEFT);
      dragging = false;
      touchReleaseTime = millis();
      touchedTime = 0;
    }
  }
}
