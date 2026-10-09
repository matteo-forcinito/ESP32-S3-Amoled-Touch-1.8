/*
 * AMOLED Watch OS - boot sequence.
 *
 *   settings (NVS) -> board (I2C, PMU, RTC, SD) -> display + LVGL -> clock
 *   -> theme + apps (home screen) -> power manager (lights the screen up)
 *   -> services (sound, radio, Wi-Fi, time sync, weather, Bluetooth)
 *
 * After this, app_main() returns: everything runs in event-driven tasks
 * that sleep when idle (see core/power.h for the power design).
 */

#include "apps/apps.h"
#include "core/app.h"
#include "core/clock.h"
#include "core/lv_port.h"
#include "core/power.h"
#include "core/settings.h"
#include "core/sys.h"
#include "hardware/board.h"
#include "companion/ble_companion.h"
#include "services/fw_update.h"
#include "services/notify.h"
#include "services/sound.h"
#include "services/time_sync.h"
#include "services/weather.h"
#include "services/wifi.h"
#include "ui/ui.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "AMOLED Watch OS starting");

    sys_check_last_reset();
    sys_worker_init();   /* background jobs for timers, first of all */
    ESP_ERROR_CHECK(settings_init());
    ESP_ERROR_CHECK(board_init());
    ESP_ERROR_CHECK(lv_port_init());
    clock_init();
    sys_heap_log("display");

    /* Services the UI reads at creation time. */
    notify_init();
    weather_init();
    sound_service_init();
    ESP_ERROR_CHECK(wifi_service_init());
    sys_heap_log("services");

    lv_port_lock();
    ui_init();
    apps_init();

    if (sys_last_reset_was_crash())
    {
        ui_toast("Riavviato dopo un errore (vedi Impostazioni > Info)");
    }

    lv_port_unlock();
    sys_heap_log("ui");

    app_manager_init();
    ESP_ERROR_CHECK(power_init());

    /* Network-facing services last: the screen is already up. */
    time_sync_init();

    if (ble_companion_init() != ESP_OK)
    {
        ESP_LOGW(TAG, "Bluetooth not available");
    }

    sys_heap_log("ready");
    ESP_LOGI(TAG, "Ready");

    /*
     * Rollback guard: a firmware that just arrived by update (or the launcher
     * re-armed by an external app) is "on trial". Only after a clean boot and
     * a few seconds of running is it kept; a crash before means the bootloader
     * starts the previous firmware again.
     */
    vTaskDelay(pdMS_TO_TICKS(5000));
    fw_update_confirm_boot();
}
