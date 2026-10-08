#include "apps_internal.h"

#include "core/state.h"
#include "services/ble_companion.h"
#include "services/radio.h"
#include "ui/ui.h"

#include <stdlib.h>

/*
 * "Now playing" (swipe right from the watch face): the web radio when it
 * plays, otherwise the music playing on the phone (remote control over BLE).
 */

typedef struct
{
    lv_obj_t *source;
    lv_obj_t *title;
    lv_obj_t *subtitle;
    lv_obj_t *play_label;
    bool radio_mode;
} np_t;


/* Each instance (home tile, Music app) has its own np_t, freed with its widgets. */
static void refresh(lv_observer_t *observer, lv_subject_t *subject)
{
    (void)subject;

    np_t *np = lv_observer_get_user_data(observer);

    ble_music_t music;
    bool phone = ble_companion_connected() && ble_companion_music(&music);
    radio_info_t info;
    int station = radio_last();

    np->radio_mode = radio_is_on() || !phone;

    if (np->radio_mode)
    {
        char song[96] = "";
        lv_label_set_text(np->source, LV_SYMBOL_AUDIO "  WEB RADIO");
        lv_label_set_text(np->title, station >= 0 && radio_get(station, &info) ? info.name : "Radio");

        if (radio_is_on() && radio_player_title(song, sizeof(song)))
        {
            lv_label_set_text(np->subtitle, song);
        }
        else
        {
            lv_label_set_text(np->subtitle, radio_is_on() ? radio_player_state_text() : "Tocca play per ascoltare");
        }

        lv_label_set_text(np->play_label, radio_is_on() ? LV_SYMBOL_STOP : LV_SYMBOL_PLAY);
    }
    else
    {
        lv_label_set_text(np->source, LV_SYMBOL_BLUETOOTH "  TELEFONO");
        lv_label_set_text(np->title, music.track[0] != '\0' ? music.track : "Musica");
        lv_label_set_text(np->subtitle, music.artist);
        lv_label_set_text(np->play_label, music.playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    }
}

static void on_prev(lv_event_t *e)
{
    np_t *np = lv_event_get_user_data(e);

    if (np->radio_mode)
    {
        radio_next(-1);
    }
    else
    {
        ble_companion_music_command("previous");
    }
}

static void on_next(lv_event_t *e)
{
    np_t *np = lv_event_get_user_data(e);

    if (np->radio_mode)
    {
        radio_next(1);
    }
    else
    {
        ble_companion_music_command("next");
    }
}

static void on_play(lv_event_t *e)
{
    np_t *np = lv_event_get_user_data(e);

    if (np->radio_mode)
    {
        radio_toggle();
    }
    else
    {
        ble_music_t music;
        ble_companion_music(&music);
        ble_companion_music_command(music.playing ? "pause" : "play");
    }
}

static lv_obj_t *control(lv_obj_t *parent, const char *icon, int32_t size, uint32_t color, lv_event_cb_t cb, np_t *np)
{
    lv_obj_t *button = lv_obj_create(parent);
    ui_make_card(button);
    lv_obj_set_clickable(button, true);
    lv_obj_set_size(button, size, size);
    lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(color), 0);
    lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, np);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, icon);
    lv_obj_set_style_text_font(label, size > 90 ? &lv_font_montserrat_32 : UI_FONT_LARGE, 0);
    lv_obj_center(label);

    return label;
}

static void free_np(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

void now_playing_create(lv_obj_t *parent)
{
    np_t *np = calloc(1, sizeof(np_t));

    if (np == NULL)
    {
        return;
    }

    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(root, 24, 0);
    lv_obj_set_style_pad_row(root, 10, 0);
    lv_obj_set_scrollable(root, false);
    lv_obj_add_event_cb(root, free_np, LV_EVENT_DELETE, np);

    np->source = lv_label_create(root);
    lv_obj_set_style_text_font(np->source, UI_FONT_SMALL, 0);
    lv_obj_set_style_text_color(np->source, ui_accent(), 0);

    np->title = lv_label_create(root);
    lv_obj_set_style_text_font(np->title, ui_font_headline, 0);
    lv_label_set_long_mode(np->title, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_width(np->title, LV_PCT(100));
    lv_obj_set_style_text_align(np->title, LV_TEXT_ALIGN_CENTER, 0);

    np->subtitle = lv_label_create(root);
    lv_label_set_long_mode(np->subtitle, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_width(np->subtitle, LV_PCT(100));
    lv_obj_set_style_text_align(np->subtitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(np->subtitle, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    lv_obj_set_style_pad_bottom(np->subtitle, 24, 0);

    lv_obj_t *row = lv_obj_create(root);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    control(row, LV_SYMBOL_PREV, 72, UI_COLOR_CARD, on_prev, np);
    np->play_label = control(row, LV_SYMBOL_PLAY, 104, UI_COLOR_PINK, on_play, np);
    control(row, LV_SYMBOL_NEXT, 72, UI_COLOR_CARD, on_next, np);

    lv_subject_add_observer_obj(state_subject(STATE_RADIO), refresh, root, np);
    lv_subject_add_observer_obj(state_subject(STATE_RADIO_VERSION), refresh, root, np);
    lv_subject_add_observer_obj(state_subject(STATE_MUSIC_VERSION), refresh, root, np);
    lv_subject_add_observer_obj(state_subject(STATE_BLE), refresh, root, np);
}
