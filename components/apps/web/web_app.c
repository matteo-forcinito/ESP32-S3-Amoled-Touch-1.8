#include "apps_internal.h"

#include "core/lv_port.h"
#include "core/sys.h"
#include "services/web_server.h"
#include "ui/ui.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

/*
 * Setup web page. While this app is open the watch serves the page and
 * shows its address as text and QR code (scan it with the phone camera).
 * Closing the app stops the server and lets Wi-Fi switch off.
 */

static lv_obj_t *s_page = NULL;
static volatile int s_generation = 0;

static void show_ready(void *arg)
{
    if ((int)(intptr_t)arg != s_generation || s_page == NULL)
    {
        return;
    }

    lv_obj_clean(s_page);

    lv_obj_t *title = lv_label_create(s_page);
    lv_label_set_text(title, "Pagina web");
    lv_obj_set_style_text_font(title, ui_font_title, 0);

    if (!web_server_running())
    {
        ui_text(s_page, web_server_last_error(), true);
        return;
    }

    char url[48];
    char ap[33];
    web_server_url(url, sizeof(url));

    if (web_server_ap_name(ap, sizeof(ap)))
    {
        char text[96];
        snprintf(text, sizeof(text), "1. Collegati alla rete Wi-Fi\n\"%s\"", ap);
        ui_text(s_page, text, false);
    }

    lv_obj_t *frame = lv_obj_create(s_page);
    lv_obj_remove_style_all(frame);
    lv_obj_set_size(frame, 220, 220);
    lv_obj_set_style_bg_color(frame, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(frame, 20, 0);

    lv_obj_t *qr = lv_qrcode_create(frame);
    lv_qrcode_set_size(qr, 190);
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
    lv_qrcode_update(qr, url, (uint32_t)strlen(url));
    lv_obj_center(qr);

    lv_obj_t *link = lv_label_create(s_page);
    lv_label_set_text(link, url);
    lv_obj_set_style_text_font(link, UI_FONT_LARGE, 0);
    lv_obj_set_style_text_color(link, ui_accent(), 0);

    ui_text(s_page, "oppure http://watch.local/  ·  chiudi l'app per spegnere il Wi-Fi", true);
}

static void start_task(void *arg)
{
    web_server_start();
    ui_async(show_ready, arg);
    vTaskDelete(NULL);
}

static void create(lv_obj_t *screen, void *arg)
{
    (void)arg;

    s_page = ui_page(screen, "Pagina web");
    lv_obj_set_flex_align(s_page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    ui_text(s_page, "Avvio Wi-Fi...", true);

    if (!sys_task_create(start_task, "web_start", 4096, (void *)(intptr_t)s_generation, 3, SYS_CORE_ANY))
    {
        ui_text(s_page, "Memoria insufficiente per avviare il server.", true);
    }
}

static void stop_task(void *arg)
{
    (void)arg;
    web_server_stop();
    vTaskDelete(NULL);
}

static void destroy(void)
{
    s_generation++;
    s_page = NULL;
    sys_task_create(stop_task, "web_stop", 4096, NULL, 3, SYS_CORE_ANY);
}

const app_t web_app = {
    .id = "web",
    .name = "Pagina web",
    .icon = LV_SYMBOL_UPLOAD,
    .color = UI_COLOR_GREEN,
    .flags = APP_FLAG_KEEP_SCREEN_ON,
    .create = create,
    .destroy = destroy,
};
