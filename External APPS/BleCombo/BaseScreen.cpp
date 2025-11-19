#include "BaseScreen.h"

void BaseScreen::createHeader(const char *mode) {
  lv_obj_t *header = lv_obj_create(scr);
  lv_obj_set_size(header, LV_PCT(100), 40);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(header, lv_color_hex(0x202020), 0);
  lv_obj_set_style_pad_all(header, 5, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lblConn = lv_label_create(header);
  lv_label_set_text(lblConn, ble->isConnected() ? "Connected" : "Disconnected");
  lv_obj_align(lblConn, LV_ALIGN_LEFT_MID, 10, 0);

  lv_obj_t *lblMode = lv_label_create(header);
  lv_label_set_text_fmt(lblMode, "Mode: %s", mode);
  lv_obj_align(lblMode, LV_ALIGN_RIGHT_MID, -10, 0);
}
