#ifndef HARDWARE_BUTTON_H
#define HARDWARE_BUTTON_H

/*
 * Physical buttons.
 *
 *   BOOT  - a GPIO: click / long press; also wakes the chip from light sleep.
 *   PWR   - on the PMU: polled with button_poll_pmu() and reported the same way.
 *
 * The callback runs in the esp_timer task: keep it short (post to a queue).
 */

#include "esp_err.h"

typedef enum
{
    BUTTON_BOOT,
    BUTTON_PWR,
} button_id_t;

typedef enum
{
    BUTTON_CLICK,
    BUTTON_LONG_PRESS,   /* fired once after ~700 ms, while still held */
} button_event_t;

typedef void (*button_cb_t)(button_id_t button, button_event_t event);

esp_err_t button_init(button_cb_t callback);

/* Read the PWR key latched by the PMU and report it (call periodically). */
void button_poll_pmu(void);

#endif
