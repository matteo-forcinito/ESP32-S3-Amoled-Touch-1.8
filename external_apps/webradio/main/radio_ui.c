#include "core/app.h"

#include "core/settings.h"
#include "core/state.h"
#include "webradio/radio.h"
#include "ui/ui.h"

#include <stdlib.h>

/*
 * Web radio.
 *
 *   ┌──────────────────────────┐
 *   │ m2o                      │  station (headline)
 *   │ Artist - Song            │  ICY title, scrolling
 *   │ In riproduzione · AAC 48 │  state / format
 *   │   ⏮     ( ■ )     ⏭     │
 *   │ ♪ ───────●──────         │  volume
 *   └──────────────────────────┘
 *   Preferite       ★ rows
 *   Tutte           rows (long press = add/remove favorite)
 *
 * This is the home screen of the external Web Radio app (BOOT = back to the
 * launcher). Nothing here blocks: commands go to the radio worker.
 */

#define MAX_ROWS 128

typedef struct
{
    lv_obj_t *station;
    lv_obj_t *song;
    lv_obj_t *status;
    lv_obj_t *play_label;
    lv_obj_t *rows[MAX_ROWS];
    int row_station[MAX_ROWS];
    int row_count;
} radio_ui_t;

static radio_ui_t *s_ui = NULL;

static void refresh(lv_observer_t *observer, lv_subject_t *subject)
{
    (void)observer;
    (void)subject;

    if (s_ui == NULL)
    {
        return;
    }

    radio_info_t info;
    int station = radio_last();
    bool on = radio_is_on();

    lv_label_set_text(s_ui->station, station >= 0 && radio_get(station, &info) ? info.name : "Scegli una radio");

    char song[96] = "";
    char format[32] = "";

    if (on && radio_player_title(song, sizeof(song)))
    {
        lv_label_set_text(s_ui->song, song);
    }
    else
    {
        lv_label_set_text(s_ui->song, "");
    }

    radio_player_format_text(format, sizeof(format));

    if (on || radio_player_state() == RADIO_STATE_FAILED)
    {
        if (format[0] != '\0' && radio_player_state() == RADIO_STATE_PLAYING)
        {
            lv_label_set_text_fmt(s_ui->status, "%s  ·  %s", radio_player_state_text(), format);
        }
        else
        {
            lv_label_set_text(s_ui->status, radio_player_state_text());
        }
    }
    else
    {
        lv_label_set_text(s_ui->status, "Ferma");
    }

    lv_obj_set_style_text_color(s_ui->status, lv_color_hex(radio_player_state() == RADIO_STATE_FAILED ? UI_COLOR_RED
                                                                                                        : UI_COLOR_TEXT_DIM), 0);
    lv_label_set_text(s_ui->play_label, on ? LV_SYMBOL_STOP : LV_SYMBOL_PLAY);

    /* Highlight the station playing now. */
    for (int i = 0; i < s_ui->row_count; i++)
    {
        bool current = on && s_ui->row_station[i] == radio_current();
        lv_obj_set_style_bg_color(s_ui->rows[i], lv_color_hex(current ? 0x2A1830 : UI_COLOR_CARD), 0);
    }
}

static void on_play(lv_event_t *e)
{
    (void)e;
    radio_toggle();
}

static void on_prev(lv_event_t *e)
{
    (void)e;
    radio_next(-1);
}

static void on_next(lv_event_t *e)
{
    (void)e;
    radio_next(1);
}

static void on_volume(lv_event_t *e)
{
    radio_set_volume(lv_slider_get_value(lv_event_get_target_obj(e)));
}

static void on_station(lv_event_t *e)
{
    int station = (int)(intptr_t)lv_event_get_user_data(e);

    if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED)
    {
        bool favorite = !radio_is_favorite(station);
        radio_set_favorite(station, favorite);
        ui_toast(favorite ? "Aggiunta ai preferiti" : "Tolta dai preferiti");
        lv_indev_wait_release(lv_indev_active());   /* no click after the long press */
        return;
    }

    radio_play(station);
}

static lv_obj_t *round_button(lv_obj_t *parent, const char *icon, int32_t size, uint32_t color, lv_event_cb_t cb)
{
    lv_obj_t *button = lv_obj_create(parent);
    ui_make_card(button);
    lv_obj_set_clickable(button, true);
    lv_obj_set_size(button, size, size);
    lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(color), 0);
    lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, icon);
    lv_obj_set_style_text_font(label, size > 80 ? &lv_font_montserrat_32 : UI_FONT_LARGE, 0);
    lv_obj_center(label);

    return label;
}

static void add_station_row(lv_obj_t *page, int station, bool favorite)
{
    radio_info_t info;

    if (s_ui->row_count >= MAX_ROWS || !radio_get(station, &info))
    {
        return;
    }

    lv_obj_t *row = ui_row(page, favorite ? LV_SYMBOL_AUDIO : NULL, UI_COLOR_PINK, info.name, info.genre, NULL, NULL);
    lv_obj_set_clickable(row, true);
    lv_obj_add_event_cb(row, on_station, LV_EVENT_CLICKED, (void *)(intptr_t)station);
    lv_obj_add_event_cb(row, on_station, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)station);

    s_ui->rows[s_ui->row_count] = row;
    s_ui->row_station[s_ui->row_count] = station;
    s_ui->row_count++;
}

static void create(lv_obj_t *screen, void *arg)
{
    (void)arg;

    s_ui = calloc(1, sizeof(radio_ui_t));

    if (s_ui == NULL)
    {
        return;
    }

    lv_obj_t *page = ui_page(screen, "Radio");

    /* now playing card */
    lv_obj_t *card = lv_obj_create(page);
    ui_make_card(card);
    lv_obj_remove_style(card, NULL, LV_STATE_PRESSED);
    lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 18, 0);
    lv_obj_set_style_pad_row(card, 6, 0);
    lv_obj_set_style_bg_grad_color(card, lv_color_hex(0x3A1238), 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);

    s_ui->station = lv_label_create(card);
    lv_obj_set_style_text_font(s_ui->station, ui_font_headline, 0);
    lv_label_set_long_mode(s_ui->station, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_width(s_ui->station, LV_PCT(100));
    lv_obj_set_style_text_align(s_ui->station, LV_TEXT_ALIGN_CENTER, 0);

    s_ui->song = lv_label_create(card);
    lv_label_set_long_mode(s_ui->song, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_width(s_ui->song, LV_PCT(100));
    lv_obj_set_style_text_align(s_ui->song, LV_TEXT_ALIGN_CENTER, 0);

    s_ui->status = lv_label_create(card);
    lv_obj_set_style_text_font(s_ui->status, UI_FONT_SMALL, 0);

    lv_obj_t *controls = lv_obj_create(card);
    lv_obj_remove_style_all(controls);
    lv_obj_set_size(controls, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_ver(controls, 8, 0);

    round_button(controls, LV_SYMBOL_PREV, 64, UI_COLOR_CARD_HI, on_prev);
    s_ui->play_label = round_button(controls, LV_SYMBOL_PLAY, 92, UI_COLOR_PINK, on_play);
    round_button(controls, LV_SYMBOL_NEXT, 64, UI_COLOR_CARD_HI, on_next);

    lv_obj_t *volume = lv_slider_create(card);
    lv_obj_set_width(volume, LV_PCT(90));
    lv_obj_set_height(volume, 10);
    lv_slider_set_range(volume, 0, 100);
    lv_slider_set_value(volume, settings_get()->volume, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(volume, lv_color_hex(UI_COLOR_PINK), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(volume, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_ext_click_area(volume, 16);
    lv_obj_add_event_cb(volume, on_volume, LV_EVENT_VALUE_CHANGED, NULL);

    /* station lists */
    int favorites[RADIO_FAVORITES_MAX];
    int favorite_count = radio_favorites(favorites, RADIO_FAVORITES_MAX);

    if (favorite_count > 0)
    {
        ui_section(page, "PREFERITE");

        for (int i = 0; i < favorite_count; i++)
        {
            add_station_row(page, favorites[i], true);
        }
    }

    ui_section(page, "TUTTE LE STAZIONI  ·  tieni premuto per i preferiti");

    for (int i = 0; i < radio_count(); i++)
    {
        add_station_row(page, i, false);
    }

    lv_subject_add_observer_obj(state_subject(STATE_RADIO), refresh, page, NULL);
    lv_subject_add_observer_obj(state_subject(STATE_RADIO_VERSION), refresh, page, NULL);
}

static void destroy(void)
{
    free(s_ui);
    s_ui = NULL;
}

const app_t radio_app = {
    .id = "radio",
    .name = "Radio",
    .icon = LV_SYMBOL_AUDIO,
    .color = UI_COLOR_PINK,
    .flags = APP_FLAG_STAY_ON_WAKE,
    .create = create,
    .destroy = destroy,
};
