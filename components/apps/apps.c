#include "apps/apps.h"

#include "apps_internal.h"

#include "core/lv_port.h"
#include "core/power.h"
#include "core/state.h"
#include "services/fw_update.h"
#include "services/alarm.h"

#include <stdlib.h>
#include <string.h>

void shell_create(void);

/* Launcher order. Hidden apps (sub-pages) are registered too. */
static const app_t *const s_apps[] = {
    &alarms_app,
    &weather_app,
    &timer_app,
    &music_app,
    &flashlight_app,
    &web_app,
    &usb_drive_app,
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
    &update_app,
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

    /* ring_app owns `alarm` from here (the open may be queued). */
    if (!app_open_app(&ring_app, alarm))
    {
        free(alarm);
    }
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

/* An update started from the web page: show its progress on the watch. */
static void on_update_phase(lv_observer_t *observer, lv_subject_t *subject)
{
    (void)observer;

    if (lv_subject_get_int(subject) == FW_UPDATE_WRITING && app_current() != &update_app)
    {
        power_wake(POWER_WAKE_OTHER);
        app_open_app(&update_app, NULL);
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

    lv_subject_add_observer(state_subject(STATE_UPDATE), on_update_phase, NULL);
}
