#include "KeyboardScreen.h"
#include <Arduino.h>
#include "Keymap_IT.h"

KeyboardScreen::KeyboardScreen(BleManager *b) : BaseScreen(b) {}

void KeyboardScreen::enter() {
  scr = lv_obj_create(NULL);
  createHeader("Keyboard");
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_scr_load(scr);

  lv_obj_t *label = lv_label_create(scr);
  lv_label_set_text(label, "Keyboard Mode");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);

  static const char *row1[] = { "Q","W","E","R","T","Y","U","I","O","P" };
  static const char *row2[] = { "A","S","D","F","G","H","J","K","L" };
  static const char *row3[] = { "Z","X","C","V","B","N","M" };
  static const char *row4[] = { "SHIFT","SPACE","DEL" };

  lv_obj_t *col = lv_obj_create(scr);
  lv_obj_set_size(col, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_align(col, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

  createKeyRow(col, row1, 10);
  createKeyRow(col, row2, 9);
  createKeyRow(col, row3, 7);
  createKeyRow(col, row4, 3);
}

void KeyboardScreen::createKeyRow(lv_obj_t *parent, const char *keys[], int count) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 60); // altezza fissa più grande
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_row(row, 4, 0);
    lv_obj_set_style_pad_column(row, 4, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < count; i++) {
        lv_obj_t *btn = lv_btn_create(row);
        lv_obj_set_flex_grow(btn, 1);   // tutti i tasti si ridimensionano equamente
        lv_obj_set_height(btn, LV_DPX(50)); // altezza tasto costante
        lv_obj_set_style_pad_top(btn, 6, 0);
        lv_obj_set_style_pad_bottom(btn, 6, 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, keys[i]);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            KeyboardScreen *self = (KeyboardScreen*)lv_event_get_user_data(e);
            lv_obj_t *lbl = lv_obj_get_child(lv_event_get_target(e), 0);
            const char *txt = lv_label_get_text(lbl);
            self->onKeyPressed(txt); // prendiamo il primo carattere
        }, LV_EVENT_CLICKED, this);
    }
}


void KeyboardScreen::onKeyPressed(const char *key) {
  if (strcmp(key, "SPACE") == 0) {
    ble->sendKey(' ');
    return;
  }
  if (strcmp(key, "DEL") == 0) {
    ble->sendSpecialKey(KEY_BACKSPACE);
    return;
  }
  if (strcmp(key, "SHIFT") == 0) {
    shift = !shift;
    return;
  }

  char c = key[0];
  if (!shift) c = tolower(c);
  ble->sendKey(c);
}

void KeyboardScreen::loop() {}
void KeyboardScreen::exit() { lv_obj_del(scr); }
