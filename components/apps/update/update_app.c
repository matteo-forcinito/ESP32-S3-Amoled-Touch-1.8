#include "apps_internal.h"

#include "core/state.h"
#include "services/fw_update.h"
#include "ui/ui.h"

#include "esp_ota_ops.h"

#include <stdlib.h>

/*
 * Settings > Aggiornamento: version, update from the PC (web page) or online.
 *
 * The screen only observes STATE_UPDATE / STATE_UPDATE_PERCENT: the transfer
 * runs in the web server or in the fw_update task, never here. When an update
 * starts from the web page, apps.c opens this screen by itself.
 */

typedef struct
{
    lv_obj_t *status;
    lv_obj_t *bar;
    lv_obj_t *check_row;
    lv_obj_t *install_row;
} update_ui_t;

static update_ui_t *s_ui = NULL;

static void on_phase(lv_observer_t *observer, lv_subject_t *subject)
{
    (void)observer;

    if (s_ui == NULL)
    {
        return;
    }

    fw_update_phase_t phase = (fw_update_phase_t)lv_subject_get_int(subject);
    const char *message = fw_update_message();
    uint32_t color = UI_COLOR_TEXT_DIM;

    lv_obj_set_hidden(s_ui->bar, phase != FW_UPDATE_WRITING && phase != FW_UPDATE_RESTARTING);
    lv_obj_set_hidden(s_ui->install_row, phase != FW_UPDATE_AVAILABLE);

    switch (phase)
    {
        case FW_UPDATE_CHECKING:
            lv_label_set_text(s_ui->status, "Controllo in corso...");
            break;
        case FW_UPDATE_UP_TO_DATE:
            lv_label_set_text(s_ui->status, "Il firmware è aggiornato.");
            color = UI_COLOR_GREEN;
            break;
        case FW_UPDATE_AVAILABLE:
            lv_label_set_text_fmt(s_ui->status, "Disponibile la versione %s.", fw_update_new_version());
            color = UI_COLOR_BLUE;
            lv_label_set_text_fmt(ui_row_value(s_ui->install_row), "%s", fw_update_new_version());
            break;
        case FW_UPDATE_WRITING:
            lv_label_set_text(s_ui->status, "Installazione: non spegnere l'orologio.");
            color = UI_COLOR_ORANGE;
            break;
        case FW_UPDATE_RESTARTING:
            lv_label_set_text(s_ui->status, "Verificato. Riavvio...");
            color = UI_COLOR_GREEN;
            break;
        case FW_UPDATE_FAILED:
            lv_label_set_text(s_ui->status, message[0] != '\0' ? message : "Aggiornamento non riuscito.");
            color = UI_COLOR_RED;
            break;
        default:
            lv_label_set_text(s_ui->status, "");
            break;
    }

    lv_obj_set_style_text_color(s_ui->status, ui_color(color), 0);
}

/* Whole percents only (fw_update publishes ~100 changes): cheap to draw. */
static void on_percent(lv_observer_t *observer, lv_subject_t *subject)
{
    (void)observer;

    if (s_ui != NULL)
    {
        lv_bar_set_value(s_ui->bar, lv_subject_get_int(subject), LV_ANIM_OFF);
    }
}

static void check(lv_event_t *e)
{
    (void)e;
    fw_update_check_online(false);
}

static void install_confirmed(bool confirmed, void *user_data)
{
    (void)user_data;

    if (confirmed)
    {
        fw_update_check_online(true);
    }
}

static void install(lv_event_t *e)
{
    (void)e;
    ui_confirm("Aggiorna", "Il firmware viene scaricato, verificato e installato; poi l'orologio si riavvia.",
               "Installa", UI_COLOR_BLUE, install_confirmed, NULL);
}

static void create(lv_obj_t *screen, void *arg)
{
    (void)arg;

    s_ui = calloc(1, sizeof(update_ui_t));

    if (s_ui == NULL)
    {
        return;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    lv_obj_t *page = ui_page(screen, "Aggiornamento");

    lv_obj_t *version = ui_text(page, "", false);
    lv_label_set_text_fmt(version, "Versione %s  ·  %s", fw_update_running_version(),
                          running != NULL ? running->label : "?");

    s_ui->status = ui_text(page, "", false);

    s_ui->bar = lv_bar_create(page);
    lv_obj_set_size(s_ui->bar, LV_PCT(100), 12);
    lv_bar_set_range(s_ui->bar, 0, 100);
    lv_obj_set_style_bg_color(s_ui->bar, ui_color(UI_COLOR_CARD_HI), 0);
    lv_obj_set_style_bg_color(s_ui->bar, ui_accent(), LV_PART_INDICATOR);

    ui_section(page, "DAL COMPUTER");
    ui_text(page, "Apri la pagina web dell'orologio, sezione \"Aggiornamento firmware\", e scegli "
                  "build/amoled_watch.bin.", true);
    ui_row(page, LV_SYMBOL_UPLOAD, UI_COLOR_BLUE, "Apri pagina web", NULL, apps_open_cb, (void *)"web");

    ui_section(page, "ONLINE");
    s_ui->check_row = ui_row(page, LV_SYMBOL_REFRESH, UI_COLOR_TEAL, "Controlla aggiornamenti", NULL, check, NULL);
    s_ui->install_row = ui_row(page, LV_SYMBOL_DOWNLOAD, UI_COLOR_GREEN, "Installa", "", install, NULL);
    ui_text(page, "L'indirizzo del file di versione si imposta dalla pagina web.", true);

    /* Observers die with their objects: nothing to remove by hand. */
    lv_subject_add_observer_obj(state_subject(STATE_UPDATE_PERCENT), on_percent, s_ui->bar, NULL);
    lv_subject_add_observer_obj(state_subject(STATE_UPDATE), on_phase, s_ui->status, NULL);
}

static void destroy(void)
{
    free(s_ui);
    s_ui = NULL;
}

const app_t update_app = {
    .id = "settings.update",
    .name = "Aggiornamento",
    .icon = LV_SYMBOL_DOWNLOAD,
    .color = UI_COLOR_GREEN,
    .flags = APP_FLAG_HIDDEN | APP_FLAG_STAY_ON_WAKE,
    .create = create,
    .destroy = destroy,
};
