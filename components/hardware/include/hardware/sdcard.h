#ifndef HARDWARE_SDCARD_H
#define HARDWARE_SDCARD_H

/*
 * microSD card, mounted at /sdcard (FAT32).
 *
 *   /sdcard/apps/<name>/<name>.bin   external apps (+ icon.png)
 *   /sdcard/config/radios.txt        web radio stations
 *   /sdcard/config/wifi.txt          known Wi-Fi networks (ssid = password)
 */

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define SDCARD_MOUNT "/sdcard"

/* Mount (idempotent). */
esp_err_t sdcard_mount(void);
void sdcard_unmount(void);
bool sdcard_is_mounted(void);

/* Card size and free space in MB (0 if not mounted). */
void sdcard_space(uint32_t *total_mb, uint32_t *free_mb);

#endif
