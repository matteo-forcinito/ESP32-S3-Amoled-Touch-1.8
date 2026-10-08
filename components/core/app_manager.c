#include "core/app.h"

#include "core/lv_port.h"
#include "core/power.h"
#include "core/state.h"

#include "esp_log.h"
#include "esp_timer.h"

#include <string.h>

static const char *TAG = "apps";

#define MAX_APPS        48
#define MAX_STACK       8
#define ANIM_MS         220
#define AUTO_HOME_MS    (2 * 60 * 1000)

typedef struct
{
    const app_t *app;
    lv_obj_t *screen;
} stack_entry_t;

static const app_t *s_apps[MAX_APPS];
static size_t s_app_count = 0;

static stack_entry_t s_stack[MAX_STACK];
static int s_depth = 0;

static lv_obj_t *s_home = NULL;
static bool (*s_home_back)(void) = NULL;
static void (*s_aod_create)(lv_obj_t *screen) = NULL;
static lv_obj_t *s_aod_screen = NULL;
static int64_t s_sleep_started_us = 0;

/* ------------------------------------------------------------ registry */

void app_register(const app_t *app)
{
    if (s_app_count < MAX_APPS && app != NULL)
    {
        s_apps[s_app_count++] = app;
    }
}

size_t app_count(void)
{
    return s_app_count;
}

const app_t *app_at(size_t index)
{
    return index < s_app_count ? s_apps[index] : NULL;
}

const app_t *app_find(const char *id)
{
    for (size_t i = 0; i < s_app_count; i++)
    {
        if (strcmp(s_apps[i]->id, id) == 0)
        {
            return s_apps[i];
        }
    }

    return NULL;
}

/* ---------------------------------------------------------- navigation */

static lv_obj_t *top_screen(void)
{
    return s_depth > 0 ? s_stack[s_depth - 1].screen : s_home;
}

static lv_obj_t *new_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, lv_color_white(), 0);
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    return screen;
}

bool app_open_app(const app_t *app, void *arg)
{
    if (app == NULL || app->create == NULL)
    {
        return false;
    }

    if (s_depth >= MAX_STACK)
    {
        ESP_LOGW(TAG, "Navigation stack full");
        return false;
    }

    if (s_depth > 0 && s_stack[s_depth - 1].app->pause != NULL)
    {
        s_stack[s_depth - 1].app->pause();
    }

    lv_obj_t *screen = new_screen();
    s_stack[s_depth].app = app;
    s_stack[s_depth].screen = screen;
    s_depth++;

    app->create(screen, arg);

    if (app->flags & APP_FLAG_KEEP_SCREEN_ON)
    {
        power_keep_screen_on(true);
    }

    lv_screen_load_anim(screen, LV_SCREEN_LOAD_ANIM_MOVE_LEFT, ANIM_MS, 0, false);
    ESP_LOGI(TAG, "Open %s", app->id);

    return true;
}

bool app_open(const char *id, void *arg)
{
    const app_t *app = app_find(id);

    if (app == NULL)
    {
        ESP_LOGW(TAG, "No app \"%s\"", id);
        return false;
    }

    return app_open_app(app, arg);
}

/* Pop the top app. `animate`: slide back to what is below. */
static void pop(bool animate)
{
    if (s_depth == 0)
    {
        return;
    }

    stack_entry_t top = s_stack[--s_depth];

    if (top.app->destroy != NULL)
    {
        top.app->destroy();
    }

    if (top.app->flags & APP_FLAG_KEEP_SCREEN_ON)
    {
        power_keep_screen_on(false);
    }

    lv_obj_t *below = top_screen();

    if (animate && s_aod_screen == NULL)
    {
        /* auto_del = true: the old screen is deleted when the slide ends. */
        lv_screen_load_anim(below, LV_SCREEN_LOAD_ANIM_MOVE_RIGHT, ANIM_MS, 0, true);
    }
    else
    {
        if (s_aod_screen == NULL)
        {
            lv_screen_load(below);
        }

        lv_obj_delete(top.screen);
    }

    if (s_depth > 0 && s_stack[s_depth - 1].app->resume != NULL)
    {
        s_stack[s_depth - 1].app->resume();
    }
}

void app_back(void)
{
    if (s_depth == 0)
    {
        if (s_home_back != NULL && s_home_back())
        {
            return;
        }

        power_sleep_now();   /* back at the watch face: like lowering the wrist */
        return;
    }

    const app_t *app = s_stack[s_depth - 1].app;

    if (app->back != NULL && app->back())
    {
        return;
    }

    pop(true);
}

void app_home(void)
{
    if (s_depth == 0)
    {
        if (s_home_back != NULL)
        {
            s_home_back();
        }

        return;
    }

    /* Destroy the hidden ones at once, slide only the visible one away. */
    while (s_depth > 1)
    {
        stack_entry_t top = s_stack[s_depth - 1];
        s_depth--;

        if (top.app->destroy != NULL)
        {
            top.app->destroy();
        }

        if (top.app->flags & APP_FLAG_KEEP_SCREEN_ON)
        {
            power_keep_screen_on(false);
        }

        /* The visible screen may be this one: swap it with the one below first. */
        if (lv_screen_active() == top.screen)
        {
            lv_screen_load(s_stack[s_depth - 1].screen);
        }

        lv_obj_delete(top.screen);
    }

    pop(true);
}

const app_t *app_current(void)
{
    return s_depth > 0 ? s_stack[s_depth - 1].app : NULL;
}

/* -------------------------------------------------------- screen states */

static void enter_aod(void)
{
    if (s_aod_screen != NULL || s_aod_create == NULL)
    {
        return;
    }

    s_aod_screen = new_screen();
    s_aod_create(s_aod_screen);
    lv_screen_load(s_aod_screen);
}

static void leave_aod(void)
{
    if (s_aod_screen == NULL)
    {
        return;
    }

    lv_screen_load(top_screen());
    lv_obj_delete(s_aod_screen);
    s_aod_screen = NULL;
}

static void screen_observer(lv_observer_t *observer, lv_subject_t *subject)
{
    (void)observer;

    power_screen_t screen = (power_screen_t)lv_subject_get_int(subject);

    if (screen == POWER_SCREEN_AOD)
    {
        s_sleep_started_us = esp_timer_get_time();
        enter_aod();
    }
    else if (screen == POWER_SCREEN_OFF)
    {
        if (s_sleep_started_us == 0)
        {
            s_sleep_started_us = esp_timer_get_time();
        }
    }
    else if (screen == POWER_SCREEN_ON)
    {
        leave_aod();

        /* After a long sleep, wake up on the watch face (like a real watch). */
        if (s_sleep_started_us != 0 &&
            esp_timer_get_time() - s_sleep_started_us > (int64_t)AUTO_HOME_MS * 1000)
        {
            const app_t *app = app_current();

            if (app != NULL && !(app->flags & APP_FLAG_STAY_ON_WAKE))
            {
                while (s_depth > 0)
                {
                    pop(false);
                }
            }

            if (s_home_back != NULL)
            {
                s_home_back();
            }
        }

        s_sleep_started_us = 0;
    }
}

static void button_handler(button_id_t button, button_event_t event)
{
    if (button == BUTTON_BOOT && event == BUTTON_CLICK)
    {
        app_back();
    }
    else if (button == BUTTON_BOOT && event == BUTTON_LONG_PRESS)
    {
        app_home();
    }
    else if (button == BUTTON_PWR && event == BUTTON_CLICK)
    {
        power_sleep_now();
    }
    else if (button == BUTTON_PWR && event == BUTTON_LONG_PRESS)
    {
        app_open("system.power", NULL);
    }
}

static void back_gesture(void)
{
    const app_t *app = app_current();

    if (app != NULL && !(app->flags & APP_FLAG_NO_BACK_GESTURE))
    {
        app_back();
    }
}

void app_manager_set_home(lv_obj_t *home, bool (*home_back)(void))
{
    s_home = home;
    s_home_back = home_back;
    lv_screen_load(home);
}

void app_manager_set_aod(void (*aod_create)(lv_obj_t *screen))
{
    s_aod_create = aod_create;
}

void app_manager_init(void)
{
    lv_port_lock();
    lv_subject_add_observer(state_subject(STATE_SCREEN), screen_observer, NULL);
    lv_port_unlock();

    lv_port_set_back_gesture_cb(back_gesture);
    power_set_button_handler(button_handler);
}
