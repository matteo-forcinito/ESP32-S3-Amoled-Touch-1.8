#include "apps_internal.h"

#include "core/clock.h"
#include "core/sys.h"
#include "core/settings.h"
#include "core/state.h"
#include "services/alarm.h"
#include "services/radio.h"
#include "services/sound.h"
#include "ui/ui.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

/*
 * The ringing alarm, full screen.
 *
 * RULE: THE ALARM MUST ALWAYS RING (same invariant as the e-paper clock).
 * With a radio chosen, the stream connects silently; the speaker goes to the
 * radio only once it is buffered. The melody takes over if the radio is not
 * ready within 25 s, fails, or goes silent for 8 s while playing.
 */

#define RADIO_START_TIMEOUT_MS  25000
#define RADIO_SILENCE_MS        8000
#define AUTO_STOP_MS            (10 * 60 * 1000)
#define SNOOZE_MINUTES          5

typedef enum
{
    SOURCE_MELODY,
    SOURCE_RADIO_STARTING,
    SOURCE_RADIO,
} source_t;

static alarm_t s_alarm;
static source_t s_source;
static uint32_t s_started_ms;
static lv_timer_t *s_timer = NULL;
static lv_obj_t *s_time = NULL;
static lv_obj_t *s_note = NULL;
static lv_obj_t *s_ring_icon = NULL;

static void stop_radio_task(void *arg)
{
    (void)arg;
    radio_player_stop();
    vTaskDelete(NULL);
}

static void stop_sound(void)
{
    sound_alarm_stop();

    if (s_source != SOURCE_MELODY)
    {
        /* radio_player_stop() may wait for the network: not in the UI task. */
        sys_task_create(stop_radio_task, "ring_stop", 3072, NULL, 4, SYS_CORE_ANY);
    }

    s_source = SOURCE_MELODY;
}

static void fall_back_to_melody(const char *why)
{
    if (s_source != SOURCE_MELODY)
    {
        stop_sound();
    }

    lv_label_set_text(s_note, why);
    sound_alarm_start();
}

static void tick(lv_timer_t *timer)
{
    (void)timer;

    uint32_t elapsed = lv_tick_elaps(s_started_ms);

    struct tm now;
    char text[12];
    clock_local(&now);
    clock_format_hm(&now, text, sizeof(text));
    lv_label_set_text(s_time, text);

    /* gentle pulse of the bell */
    lv_obj_set_style_transform_rotation(s_ring_icon, (elapsed / 120) % 2 ? 120 : -120, 0);

    if (s_source == SOURCE_RADIO_STARTING)
    {
        radio_state_t st = radio_player_state();

        if (st == RADIO_STATE_READY || st == RADIO_STATE_PLAYING)
        {
            radio_player_play();
            s_source = SOURCE_RADIO;
            lv_label_set_text(s_note, "");
        }
        else if (st == RADIO_STATE_FAILED || elapsed > RADIO_START_TIMEOUT_MS)
        {
            fall_back_to_melody("Radio non disponibile");
        }
    }
    else if (s_source == SOURCE_RADIO)
    {
        if (radio_player_state() == RADIO_STATE_FAILED || radio_player_silence_ms() > RADIO_SILENCE_MS)
        {
            fall_back_to_melody("Radio interrotta");
        }
    }

    if (elapsed > AUTO_STOP_MS)
    {
        stop_sound();
        alarm_dismiss(s_alarm.id);
        app_back();
    }
}

static void on_snooze(lv_event_t *e)
{
    (void)e;
    stop_sound();
    alarm_snooze(s_alarm.id, SNOOZE_MINUTES);
    ui_toast("Posticipata di 5 minuti");
    app_back();
}

static void on_stop(lv_event_t *e)
{
    (void)e;
    stop_sound();
    alarm_dismiss(s_alarm.id);
    app_back();
}

static void create(lv_obj_t *screen, void *arg)
{
    s_alarm = *(const alarm_t *)arg;
    s_started_ms = lv_tick_get();

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1A0E00), 0);
    lv_obj_set_style_bg_grad_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(screen, 24, 0);
    lv_obj_set_style_pad_row(screen, 10, 0);

    s_ring_icon = lv_label_create(screen);
    lv_label_set_text(s_ring_icon, LV_SYMBOL_BELL);
    lv_obj_set_style_text_font(s_ring_icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(s_ring_icon, lv_color_hex(UI_COLOR_ORANGE), 0);
    lv_obj_set_style_transform_pivot_x(s_ring_icon, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(s_ring_icon, 0, 0);

    s_time = lv_label_create(screen);
    lv_obj_set_style_text_font(s_time, ui_font_clock, 0);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, s_alarm.label[0] != '\0' ? s_alarm.label : "Sveglia");
    lv_obj_set_style_text_font(label, UI_FONT_LARGE, 0);

    s_note = lv_label_create(screen);
    lv_label_set_text(s_note, "");
    lv_obj_set_style_text_color(s_note, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    lv_obj_set_style_text_font(s_note, UI_FONT_SMALL, 0);

    lv_obj_t *snooze = ui_button(screen, "Posticipa", UI_COLOR_CARD_HI, on_snooze, NULL);
    lv_obj_set_style_margin_top(snooze, 20, 0);
    ui_button(screen, "Stop", UI_COLOR_ORANGE, on_stop, NULL);

    /* Start the sound. */
    radio_info_t info;

    if (radio_is_on())
    {
        radio_stop();   /* the melody waits for the speaker */
    }

    if (s_alarm.radio >= 0 && radio_get(s_alarm.radio, &info) &&
        radio_player_start(info.url, settings_get()->volume < 50 ? 60 : settings_get()->volume, false) == ESP_OK)
    {
        s_source = SOURCE_RADIO_STARTING;
        lv_label_set_text_fmt(s_note, "Sintonizzo %s...", info.name);
    }
    else
    {
        s_source = SOURCE_MELODY;
        sound_alarm_start();
    }

    s_timer = lv_timer_create(tick, 250, NULL);
    tick(s_timer);
}

static void destroy(void)
{
    if (s_timer != NULL)
    {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }

    stop_sound();
}

/* Back (BOOT / swipe) = stop; the app manager then closes the screen. */
static bool back(void)
{
    stop_sound();
    alarm_dismiss(s_alarm.id);
    return false;
}

const app_t ring_app = {
    .id = "system.ring",
    .name = "Sveglia",
    .icon = LV_SYMBOL_BELL,
    .color = UI_COLOR_ORANGE,
    .flags = APP_FLAG_HIDDEN | APP_FLAG_KEEP_SCREEN_ON | APP_FLAG_STAY_ON_WAKE,
    .create = create,
    .destroy = destroy,
    .back = back,
};
