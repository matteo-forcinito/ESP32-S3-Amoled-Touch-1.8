#ifndef HARDWARE_BOARD_H
#define HARDWARE_BOARD_H

/*
 * The board: what this device has, and the one call that wakes it all up.
 *
 * Everything above the hardware layer (core, services, apps) asks
 * board_info() instead of using pins or #ifdefs, so the same apps run on
 * any supported Waveshare board.
 *
 *     board_init()
 *       ├─ I2C bus           (shared by touch, PMU, RTC, codec, expander)
 *       ├─ IO expander       (display/touch reset lines, if present)
 *       ├─ PMU               (battery, charger, PWR key)
 *       ├─ RTC               (time kept while the ESP32 is off)
 *       └─ SD card
 *
 * Display, touch and buttons are started by the core (LVGL port and power
 * manager), because they need callbacks from it.
 */

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct
{
    const char *name;
    uint16_t width;
    uint16_t height;
    bool round;           /* round panel: keep content away from the corners */
    bool has_touch;
    bool has_pmu;
    bool has_rtc;
    bool has_audio;
    bool has_sdcard;
} board_info_t;

/* Bring up the buses and the always-on chips. Call once, early in app_main(). */
esp_err_t board_init(void);

const board_info_t *board_info(void);

#endif
