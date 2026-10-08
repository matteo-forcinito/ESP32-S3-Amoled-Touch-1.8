#ifndef HARDWARE_PMU_H
#define HARDWARE_PMU_H

/*
 * Power management unit (AXP2101): battery charger, fuel gauge and PWR key.
 *
 * The PWR key is wired to the PMU, not to a GPIO: the PMU latches presses in
 * its interrupt registers and pmu_poll_key() reads and clears them. Holding
 * PWR for ~6 s switches the board off in hardware, whatever the firmware does.
 */

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum
{
    PMU_KEY_NONE,
    PMU_KEY_SHORT,
    PMU_KEY_LONG,
} pmu_key_t;

esp_err_t pmu_init(void);

bool pmu_present(void);

/* 0..100, or -1 if unknown (no battery / no PMU). */
int pmu_battery_percent(void);

/* Battery voltage in millivolts, 0 if unknown. */
int pmu_battery_mv(void);

bool pmu_battery_present(void);
bool pmu_is_charging(void);
bool pmu_usb_present(void);

/* Last PWR key press since the previous call. */
pmu_key_t pmu_poll_key(void);

/* Cut the power (like holding PWR). The board starts again with a PWR press. */
void pmu_power_off(void);

#endif
