#include "apps_internal.h"

#include "services/extapp.h"
#include "ui/ui.h"

#include <stdlib.h>
#include <string.h>

/*
 * App launcher: a grid of round icons, three per row, in sections.
 *
 *     App                     built-in apps (from the registry)
 *     ( ♪ )  ( ⏰ )  ( ☁ )
 *     Radio  Sveglie Meteo
 *     Scheda SD               external apps found in /sdcard/apps, with their icons
 *     ( 🖼 )  ( 🖼 )
 *
 * The SD section is rebuilt every time the launcher tile is shown, so a card
 * swapped or an app uploaded from the web page appears without restarting.
 */

#define ICON_SIZE   76
#define CELL_WIDTH  104

static lv_obj_t *s_page = NULL;
static lv_obj_t *s_sd_grid = NULL;
static lv_obj_t *s_sd_title = NULL;
static extapp_t *s_ext = NULL;
static int s_ext_count = 0;

static lv_obj_t *grid(lv_obj_t *parent)
{
    lv_obj_t *g = lv_obj_create(parent);
    lv_obj_remove_style_all(g);
    lv_obj_set_size(g, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(g, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(g, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(g, 14, 0);
    lv_obj_set_scrollable(g, false);
    return g;
}

static lv_obj_t *cell(lv_obj_t *parent, const char *icon, uint32_t color, const char *name, lv_event_cb_t cb,
                      void *user_data)
{
    lv_obj_t *c = lv_obj_create(parent);
    ui_make_card(c);
    lv_obj_set_clickable(c, true);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(c, 2, 0);
    lv_obj_set_size(c, CELL_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(c, 6, 0);
    lv_obj_add_event_cb(c, cb, LV_EVENT_CLICKED, user_data);

    lv_obj_t *bubble = ui_icon_bubble(c, icon, color, ICON_SIZE);
    lv_obj_set_clickable(bubble, false);

    lv_obj_t *label = lv_label_create(c);
    lv_label_set_text(label, name);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(label, CELL_WIDTH);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);

    return c;
}

static void open_app(lv_event_t *e)
{
    app_open_app(lv_event_get_user_data(e), NULL);
}

static void open_external(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);

    if (s_ext == NULL || index >= s_ext_count)
    {
        return;
    }

    extapp_t *copy = malloc(sizeof(extapp_t));   /* owned by the run screen */

    if (copy != NULL)
    {
        *copy = s_ext[index];
        app_open_app(&extapps_run_app, copy);
    }
}

void launcher_refresh(void)
{
    if (s_sd_grid == NULL)
    {
        return;
    }

    if (s_ext == NULL)
    {
        s_ext = calloc(EXTAPP_MAX, sizeof(extapp_t));
    }

    lv_obj_clean(s_sd_grid);
    s_ext_count = s_ext != NULL ? extapp_scan(s_ext, EXTAPP_MAX) : 0;

    for (int i = 0; i < s_ext_count; i++)
    {
        const extapp_t *app = &s_ext[i];
        cell(s_sd_grid, app->icon[0] != '\0' ? app->icon : LV_SYMBOL_DIRECTORY, UI_COLOR_PURPLE, app->name,
             open_external, (void *)(intptr_t)i);
    }

    lv_obj_set_hidden(s_sd_title, s_ext_count == 0);
    lv_obj_set_hidden(s_sd_grid, s_ext_count == 0);
}

void launcher_create(lv_obj_t *parent)
{
    s_page = ui_page(parent, "App");

    lv_obj_t *apps = grid(s_page);

    for (size_t i = 0; i < app_count(); i++)
    {
        const app_t *app = app_at(i);

        if (!(app->flags & APP_FLAG_HIDDEN))
        {
            cell(apps, app->icon, app->color, app->name, open_app, (void *)app);
        }
    }

    s_sd_title = ui_section(s_page, "SCHEDA SD");
    s_sd_grid = grid(s_page);
    launcher_refresh();
}
