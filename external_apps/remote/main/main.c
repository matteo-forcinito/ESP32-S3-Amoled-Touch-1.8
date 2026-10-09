/*
 * Remote Control - external app for AMOLED Watch OS.
 *
 * The watch becomes a Bluetooth or USB keyboard (and later mouse, media
 * remote, gamepad...). The keyboard supports swipe typing: slide the finger
 * over the letters without lifting it.
 *
 * Installed from /sdcard/apps/Remote/Remote.bin. BOOT on the home screen =
 * back to the launcher; any restart or crash also returns to the launcher.
 */

#include "remote.h"

#include "core/lv_port.h"
#include "core/power.h"
#include "core/settings.h"
#include "core/sys.h"
#include "extapp_sdk.h"
#include "hardware/board.h"
#include "ui/ui.h"

#include "esp_log.h"

static const char *TAG = "remote_app";

static bool home_back(void)
{
    hid_link_stop();
    extapp_return_to_launcher();
    return true;
}

void app_main(void)
{
    extapp_sdk_init();   /* any restart from now on goes back to the launcher */
    sys_check_last_reset();
    sys_worker_init();

    ESP_ERROR_CHECK(settings_init());
    ESP_ERROR_CHECK(board_init());
    ESP_ERROR_CHECK(lv_port_init());

    remote_settings_load();
    remote_dictionary();   /* index the words now (~0.2 s), not at the first swipe */
    remote_connect();
    sys_heap_log("hid");

    lv_port_lock();
    ui_init();
    app_register(&keyboard_app);
    app_register(&remote_options_app);

    lv_obj_t *home = lv_obj_create(NULL);
    lv_obj_remove_style_all(home);
    lv_obj_set_size(home, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(home, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(home, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(home, lv_color_white(), 0);
    remote_home_create(home);

    app_manager_set_home(home, home_back);
    lv_port_unlock();

    app_manager_init();
    ESP_ERROR_CHECK(power_init());

    sys_heap_log("ready");
    ESP_LOGI(TAG, "Remote Control ready");
}
