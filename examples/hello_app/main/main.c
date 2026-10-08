/*
 * Hello: the smallest external app.
 *
 * It reuses the platform components (board, LVGL port with power saving,
 * theme, widgets, app manager), so it looks and behaves like the built-in
 * apps, and it would run unchanged on any supported board.
 *
 * BOOT click or swipe back = return to the launcher.
 */

#include "core/app.h"
#include "core/lv_port.h"
#include "core/power.h"
#include "core/settings.h"
#include "extapp_sdk.h"
#include "hardware/board.h"
#include "ui/ui.h"

static int s_count = 0;

static void on_tap(lv_event_t *e)
{
    lv_obj_t *row = lv_event_get_current_target_obj(e);
    lv_label_set_text_fmt(ui_row_value(row), "%d", ++s_count);
}

/* "Back" on our only screen: leave the app. */
static bool home_back(void)
{
    extapp_return_to_launcher();
    return true;
}

void app_main(void)
{
    extapp_sdk_init();   /* any restart from now on goes back to the launcher */

    settings_init();
    board_init();
    lv_port_init();

    lv_port_lock();
    ui_init();

    lv_obj_t *home = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(home, lv_color_black(), 0);
    lv_obj_t *page = ui_page(home, "Hello!");
    ui_text(page, "Un'app esterna costruita sugli stessi componenti del launcher.", true);
    ui_row(page, LV_SYMBOL_PLUS, UI_COLOR_GREEN, "Tocca qui", "0", on_tap, NULL);
    ui_button(page, "Torna al launcher", UI_COLOR_BLUE, (lv_event_cb_t)extapp_return_to_launcher, NULL);

    app_manager_set_home(home, home_back);
    lv_port_unlock();

    app_manager_init();
    power_init();
}
