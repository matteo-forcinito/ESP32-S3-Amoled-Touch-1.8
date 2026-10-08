#include "apps/apps.h"

#include "apps_internal.h"

#include "core/lv_port.h"
#include "core/power.h"
#include "services/alarm.h"

#include <stdlib.h>
#include <string.h>

void shell_create(void);

/* Launcher order. Hidden apps (sub-pages) are registered too. */
static const app_t *const s_apps[] = {
    &radio_app,
    &alarms_app,
    &weather_app,
    &timer_app,
    &music_app,
    &flashlight_app,
    &web_app,
    &settings_app,
    /* hidden */
    &alarm_edit_app,
    &ring_app,
    &extapps_run_app,
    &settings_display_app,
    &settings_sound_app,
    &settings_wifi_app,
    &settings_bluetooth_app,
    &settings_time_app,
    &settings_about_app,
    &power_app,
    &call_app,
};

void apps_open_cb(lv_event_t *e)
{
    const char *id = lv_event_get_user_data(e);
    app_open(id, NULL);
}

/* ---- alarms ring from the esp_timer task: hop to the lvgl task */

static void open_ring(void *arg)
{
    alarm_t *alarm = arg;

    if (app_current() == &ring_app)
    {
        app_back();   /* a second alarm replaces the first */
    }

    app_open_app(&ring_app, alarm);
    free(alarm);
}

static void on_alarm(const alarm_t *alarm)
{
    alarm_t *copy = malloc(sizeof(alarm_t));

    if (copy == NULL)
    {
        return;
    }

    *copy = *alarm;
    power_wake(POWER_WAKE_ALARM);

    if (!ui_async(open_ring, copy))
    {
        free(copy);
    }
}

void apps_init(void)
{
    for (size_t i = 0; i < sizeof(s_apps) / sizeof(s_apps[0]); i++)
    {
        app_register(s_apps[i]);
    }

    /* Before the shell: the watch face asks for the next alarm as soon as it exists. */
    alarm_service_init(on_alarm);
    shell_create();
}
