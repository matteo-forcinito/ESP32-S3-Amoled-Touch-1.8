#include "core/lv_port.h"

#include "core/power.h"
#include "core/state.h"

#include "hardware/display.h"
#include "hardware/touch.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <stdlib.h>

static const char *TAG = "lv_port";

#define TASK_STACK        (8 * 1024)
#define TASK_PRIORITY     4
#define TASK_CORE         1
#define MAX_SLEEP_MS      1000
#define ASYNC_QUEUE_LEN   16
#define BOOST_HOLD_MS     60      /* keep 240 MHz between frames of an animation */

/* Edge swipe = "back", like on most smartwatches. */
#define EDGE_ZONE_PX      28
#define EDGE_SWIPE_DX     70
#define EDGE_SWIPE_MAX_DY 60

typedef struct
{
    ui_async_fn_t fn;
    void *arg;
} async_item_t;

static lv_display_t *s_disp = NULL;
static lv_indev_t *s_indev = NULL;
static TaskHandle_t s_task = NULL;
static SemaphoreHandle_t s_flush_done = NULL;
static QueueHandle_t s_async = NULL;

static volatile bool s_running = true;
static volatile bool s_touch_irq = false;
static void (*s_back_cb)(void) = NULL;

/* Touch tracking (only used in the lvgl task). */
static int s_released_reads = 0;
static bool s_swallow_until_release = false;
static bool s_edge_candidate = false;
static bool s_back_gesture = false;
static int16_t s_start_x = 0;
static int16_t s_start_y = 0;
static bool s_was_pressed = false;

/* ------------------------------------------------------------- display */

static void flush_done_isr(void *ctx)
{
    (void)ctx;

    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_flush_done, &woken);
    portYIELD_FROM_ISR(woken);
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *pixels)
{
    (void)disp;
    display_draw(area->x1, area->y1, area->x2, area->y2, pixels);
}

/* Instead of spinning while the DMA works, sleep until its interrupt. */
static void flush_wait_cb(lv_display_t *disp)
{
    (void)disp;
    xSemaphoreTake(s_flush_done, pdMS_TO_TICKS(100));
}

/* The SH8601 only accepts windows starting on even and ending on odd pixels. */
static void rounder_cb(lv_event_t *e)
{
    lv_area_t *area = lv_event_get_invalidated_area(e);

    area->x1 &= ~1;
    area->y1 &= ~1;
    area->x2 |= 1;
    area->y2 |= 1;
}

/* --------------------------------------------------------------- touch */

static void touch_irq(void *ctx)
{
    (void)ctx;

    s_touch_irq = true;

    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_task, &woken);
    portYIELD_FROM_ISR(woken);
}

static void stop_touch_polling(void)
{
    lv_timer_pause(lv_indev_get_read_timer(s_indev));
    touch_irq_arm();
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    uint16_t x = 0;
    uint16_t y = 0;
    bool pressed = touch_read(&x, &y);

    if (!pressed)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        s_was_pressed = false;
        s_edge_candidate = false;
        s_swallow_until_release = false;

        /* A few quiet reads in a row: stop polling, the INT pin takes over. */
        if (++s_released_reads >= 3 && !touch_pin_active())
        {
            s_released_reads = 0;
            stop_touch_polling();
        }

        return;
    }

    s_released_reads = 0;
    power_user_activity();

    if (!s_was_pressed)
    {
        s_was_pressed = true;
        s_start_x = (int16_t)x;
        s_start_y = (int16_t)y;
        s_edge_candidate = x < EDGE_ZONE_PX;
    }
    else if (s_edge_candidate && (int)x - s_start_x > EDGE_SWIPE_DX &&
             abs((int)y - s_start_y) < EDGE_SWIPE_MAX_DY)
    {
        s_edge_candidate = false;
        s_back_gesture = true;
        s_swallow_until_release = true;
    }

    if (s_swallow_until_release)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    data->point.x = (int32_t)x;
    data->point.y = (int32_t)y;
    data->state = LV_INDEV_STATE_PRESSED;
}

/* ------------------------------------------------------------- task */

static uint32_t tick_cb(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void handle_touch_irq(void)
{
    if (!s_touch_irq)
    {
        return;
    }

    s_touch_irq = false;

    power_screen_t screen = power_screen();

    if (screen == POWER_SCREEN_OFF || screen == POWER_SCREEN_AOD)
    {
        /* Screen off or always-on face: the touch only means "wake up". */
        power_touch_wake();
        s_swallow_until_release = true;
    }

    lv_timer_t *read_timer = lv_indev_get_read_timer(s_indev);
    lv_timer_resume(read_timer);
    lv_timer_ready(read_timer);
}

static void run_async_items(void)
{
    async_item_t item;

    while (xQueueReceive(s_async, &item, 0) == pdTRUE)
    {
        item.fn(item.arg);
    }
}

static void lvgl_task(void *arg)
{
    (void)arg;

    bool boosted = false;

    while (true)
    {
        uint32_t wait_ms = LV_NO_TIMER_READY;

        /* Full CPU speed while there is drawing to do. */
        if (!boosted)
        {
            power_cpu_boost(true);
            boosted = true;
        }

        lv_lock();

        handle_touch_irq();
        state_apply_pending();
        run_async_items();

        if (s_back_gesture)
        {
            s_back_gesture = false;
            lv_indev_reset(s_indev, NULL);
            lv_indev_wait_release(s_indev);

            if (s_back_cb != NULL)
            {
                s_back_cb();
            }
        }

        if (s_running)
        {
            wait_ms = lv_timer_handler();
        }
        else
        {
            /* Not drawing, but keep reading the touch while a finger is down. */
            lv_timer_t *read_timer = lv_indev_get_read_timer(s_indev);

            if (!lv_timer_get_paused(read_timer))
            {
                lv_indev_read(s_indev);
                wait_ms = 30;
            }
        }

        lv_unlock();

        /* Long pause ahead (no animation running): let the CPU slow down and sleep. */
        if (boosted && (wait_ms == LV_NO_TIMER_READY || wait_ms > BOOST_HOLD_MS))
        {
            power_cpu_boost(false);
            boosted = false;
        }

        TickType_t ticks;

        if (wait_ms == LV_NO_TIMER_READY)
        {
            ticks = portMAX_DELAY;   /* nothing scheduled: sleep until woken */
        }
        else
        {
            if (wait_ms > MAX_SLEEP_MS)
            {
                wait_ms = MAX_SLEEP_MS;
            }

            ticks = pdMS_TO_TICKS(wait_ms);

            if (ticks == 0)
            {
                ticks = 1;
            }
        }

        ulTaskNotifyTake(pdTRUE, ticks);
    }
}

/* ------------------------------------------------------------- public */

esp_err_t lv_port_init(void)
{
    const uint16_t width = display_width();
    const uint16_t height = display_height();

    s_flush_done = xSemaphoreCreateBinary();
    s_async = xQueueCreate(ASYNC_QUEUE_LEN, sizeof(async_item_t));

    if (s_flush_done == NULL || s_async == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = display_init(flush_done_isr, NULL);

    if (err != ESP_OK)
    {
        return err;
    }

    lv_init();
    lv_tick_set_cb(tick_cb);

    /* Two partial buffers in internal DMA RAM: LVGL draws one while the other is sent. */
    const size_t buffer_bytes = (size_t)width * CONFIG_BOARD_DRAW_BUFFER_LINES * sizeof(uint16_t);
    void *buf1 = heap_caps_malloc(buffer_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void *buf2 = heap_caps_malloc(buffer_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (buf1 == NULL || buf2 == NULL)
    {
        ESP_LOGE(TAG, "No internal RAM for the draw buffers");
        return ESP_ERR_NO_MEM;
    }

    s_disp = lv_display_create(width, height);
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565_SWAPPED);   /* panel wants big endian */
    lv_display_set_buffers(s_disp, buf1, buf2, buffer_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_disp, flush_cb);
    lv_display_set_flush_wait_cb(s_disp, flush_wait_cb);
    lv_display_add_event_cb(s_disp, rounder_cb, LV_EVENT_INVALIDATE_AREA, NULL);

    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, touch_read_cb);
    lv_indev_set_display(s_indev, s_disp);
    lv_indev_set_scroll_throw(s_indev, 6);   /* a bit more inertia than default: feels fluid */
    lv_timer_set_period(lv_indev_get_read_timer(s_indev), 15);   /* 66 Hz while a finger is down */

    state_init();

    if (xTaskCreatePinnedToCore(lvgl_task, "lvgl", TASK_STACK, NULL, TASK_PRIORITY, &s_task, TASK_CORE) != pdPASS)
    {
        return ESP_ERR_NO_MEM;
    }

    err = touch_init(touch_irq, NULL);

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Touch init failed: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "LVGL %d.%d.%d, %dx%d, 2 x %u KB draw buffers", lv_version_major(), lv_version_minor(),
             lv_version_patch(), width, height, (unsigned)(buffer_bytes / 1024));

    return ESP_OK;
}

lv_display_t *lv_port_display(void)
{
    return s_disp;
}

lv_indev_t *lv_port_indev(void)
{
    return s_indev;
}

void lv_port_lock(void)
{
    lv_lock();
}

void lv_port_unlock(void)
{
    lv_unlock();
    lv_port_wake();
}

void lv_port_wake(void)
{
    if (s_task != NULL)
    {
        xTaskNotifyGive(s_task);
    }
}

bool ui_async(ui_async_fn_t fn, void *arg)
{
    async_item_t item = {.fn = fn, .arg = arg};

    if (s_async == NULL || xQueueSend(s_async, &item, pdMS_TO_TICKS(50)) != pdTRUE)
    {
        ESP_LOGW(TAG, "ui_async queue full");
        return false;
    }

    lv_port_wake();

    return true;
}

void lv_port_set_running(bool running)
{
    if (running == s_running)
    {
        return;
    }

    lv_lock();
    s_running = running;

    if (running)
    {
        /* Things may have changed while we were not drawing: redraw it all. */
        lv_obj_invalidate(lv_screen_active());
    }

    lv_unlock();
    lv_port_wake();
}

bool lv_port_is_running(void)
{
    return s_running;
}

void lv_port_refresh_now(void)
{
    lv_lock();
    lv_refr_now(s_disp);
    lv_unlock();
}

void lv_port_set_back_gesture_cb(void (*cb)(void))
{
    s_back_cb = cb;
}
