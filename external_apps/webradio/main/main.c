/*
 * Web Radio - external app for AMOLED Watch OS.
 *
 * Installed from /sdcard/apps/WebRadio/WebRadio.bin through the launcher.
 * It reuses the platform (board, LVGL port with power saving, theme,
 * widgets, Wi-Fi) and the components/webradio engine: HTTPS, HLS, MP3/AAC.
 *
 * BOOT click (or back) = stop and return to the launcher. Any crash or
 * restart also returns to the launcher (extapp_sdk_init).
 */

#include "core/app.h"
#include "core/clock.h"
#include "core/lv_port.h"
#include "core/power.h"
#include "core/settings.h"
#include "core/sys.h"
#include "extapp_sdk.h"
#include "hardware/board.h"
#include "services/wifi.h"
#include "ui/ui.h"
#include "webradio/radio.h"

#include "esp_log.h"

static const char *TAG = "webradio_app";

extern const app_t radio_app;   /* radio_ui.c */

/* Back on the radio page: leave the app. */
static bool home_back(void)
{
    radio_stop();
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
    clock_init();

    ESP_ERROR_CHECK(wifi_service_init());
    radio_service_init();
    sys_heap_log("services");

    lv_port_lock();
    ui_init();

    lv_obj_t *home = lv_obj_create(NULL);
    lv_obj_remove_style_all(home);
    lv_obj_set_size(home, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(home, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(home, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(home, lv_color_white(), 0);
    radio_app.create(home, NULL);

    app_manager_set_home(home, home_back);
    lv_port_unlock();

    app_manager_init();
    ESP_ERROR_CHECK(power_init());

    sys_heap_log("ready");
    ESP_LOGI(TAG, "Web Radio ready");
}
