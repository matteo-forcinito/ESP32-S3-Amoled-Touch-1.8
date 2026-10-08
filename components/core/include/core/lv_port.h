#ifndef CORE_LV_PORT_H
#define CORE_LV_PORT_H

/*
 * LVGL port: connects the LVGL graphics library to the panel and the touch
 * chip, and runs it in its own task ("lvgl").
 *
 * Built to sleep: the task blocks until LVGL really has something to do.
 *
 *   touch INT ──► wake task ──► read touch at 60 Hz while the finger is down
 *                               └─ finger lifted: stop reading, wait for INT
 *   animation / invalidated area ──► LVGL timers ──► render ──► QSPI DMA
 *   nothing to do ──────────────► task blocked, CPU free to light-sleep
 *
 * THREADING RULE: LVGL is not thread safe. UI code runs in the lvgl task
 * (event callbacks, app create/destroy, ui_async() callbacks). Any other
 * task must either wrap LVGL calls in lv_port_lock()/lv_port_unlock() or,
 * better, post work with ui_async() (never blocks, no deadlocks).
 */

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "lvgl.h"

/* Start display, touch, LVGL and the lvgl task. */
esp_err_t lv_port_init(void);

lv_display_t *lv_port_display(void);
lv_indev_t *lv_port_indev(void);

void lv_port_lock(void);
void lv_port_unlock(void);

/* Wake the lvgl task (after changing something from another task). */
void lv_port_wake(void);

/* Run `fn(arg)` in the lvgl task, soon. Safe from any task. */
typedef void (*ui_async_fn_t)(void *arg);
bool ui_async(ui_async_fn_t fn, void *arg);

/*
 * Stop / restart rendering (used by the power manager while the panel is
 * asleep: nothing is drawn, touches are not passed to the UI).
 */
void lv_port_set_running(bool running);
bool lv_port_is_running(void);

/* Render everything that changed right now (lvgl task or with the lock held). */
void lv_port_refresh_now(void);

/* Called (in the lvgl task) for a "swipe right from the left edge". */
void lv_port_set_back_gesture_cb(void (*cb)(void));

#endif
