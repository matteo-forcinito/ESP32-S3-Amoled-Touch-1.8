/*
 * Remote Control options: host keyboard layout, swipe dictionary, typing
 * helpers, forget paired devices.
 */

#include "remote.h"

#include "ui/ui.h"

static const char *layout_name(uint8_t layout)
{
    return layout == HID_HOST_US ? "Inglese (US)" : "Italiano";
}

static const char *dict_name(uint8_t dict)
{
    return dict == DICT_EN ? "English" : "Italiano";
}

static void cycle_layout(lv_event_t *e)
{
    remote_settings_t s = *remote_settings();
    s.host_layout = s.host_layout == HID_HOST_US ? HID_HOST_IT : HID_HOST_US;
    remote_settings_save(&s);
    lv_label_set_text(ui_row_value(lv_event_get_current_target_obj(e)), layout_name(s.host_layout));
}

static void cycle_dict(lv_event_t *e)
{
    remote_dict_t next = remote_settings()->dictionary == DICT_EN ? DICT_IT : DICT_EN;
    remote_dictionary_select(next);
    lv_label_set_text(ui_row_value(lv_event_get_current_target_obj(e)), dict_name((uint8_t)next));
}

static bool switch_on(lv_event_t *e)
{
    return lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED);
}

static void set_swipe(lv_event_t *e)
{
    remote_settings_t s = *remote_settings();
    s.swipe = switch_on(e);
    remote_settings_save(&s);
}

static void set_auto_caps(lv_event_t *e)
{
    remote_settings_t s = *remote_settings();
    s.auto_caps = switch_on(e);
    remote_settings_save(&s);
}

static void forget_confirmed(bool confirmed, void *user_data)
{
    (void)user_data;

    if (confirmed)
    {
        hid_link_forget_hosts();
        ui_toast("Associazioni cancellate");
    }
}

static void forget_hosts(lv_event_t *e)
{
    (void)e;
    ui_confirm("Dimentica dispositivi", "Computer e telefoni dovranno associare di nuovo l'orologio.",
               "Dimentica", UI_COLOR_RED, forget_confirmed, NULL);
}

static void create(lv_obj_t *screen, void *arg)
{
    (void)arg;
    const remote_settings_t *s = remote_settings();

    lv_obj_t *page = ui_page(screen, "Opzioni");

    ui_section(page, "TASTIERA DEL COMPUTER");
    ui_row(page, LV_SYMBOL_KEYBOARD, UI_COLOR_BLUE, "Layout", layout_name(s->host_layout), cycle_layout, NULL);
    ui_text(page, "Lo stesso layout impostato sul computer: serve per accenti e simboli.", true);

    ui_section(page, "SCRITTURA");
    ui_row(page, LV_SYMBOL_LIST, UI_COLOR_ORANGE, "Dizionario", dict_name(s->dictionary), cycle_dict, NULL);
    ui_switch_row(page, LV_SYMBOL_SHUFFLE, UI_COLOR_PURPLE, "Scrittura a scorrimento", s->swipe, set_swipe, NULL);
    ui_switch_row(page, LV_SYMBOL_UP, UI_COLOR_TEAL, "Maiuscola automatica", s->auto_caps, set_auto_caps, NULL);

    ui_section(page, "BLUETOOTH");
    ui_row(page, LV_SYMBOL_TRASH, UI_COLOR_RED, "Dimentica dispositivi", NULL, forget_hosts, NULL);
}

const app_t remote_options_app = {
    .id = "remote.options",
    .name = "Opzioni",
    .icon = LV_SYMBOL_SETTINGS,
    .color = UI_COLOR_GRAY,
    .flags = APP_FLAG_HIDDEN,
    .create = create,
};
