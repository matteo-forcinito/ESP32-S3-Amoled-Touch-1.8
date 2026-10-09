#ifndef SERVICES_FW_UPDATE_H
#define SERVICES_FW_UPDATE_H

/*
 * Firmware update of the base system, without a USB cable.
 *
 *   from the PC    web page -> "Aggiornamento firmware" -> build/amoled_watch.bin
 *                  (the web server streams it in with fw_update_begin/write/end)
 *   online         settings.update_url points to a small JSON manifest:
 *                      {"version": "1.2.0", "url": "https://.../amoled_watch.bin"}
 *                  fw_update_check_online() compares the version and installs it
 *
 * The new firmware goes into the other app slot (the one external apps use),
 * then the watch restarts into it. Safety:
 *   - the file is checked BEFORE the slot is touched: right chip and project
 *     name "amoled_watch" (an external app or a merged .bin is refused)
 *   - esp_ota_end() verifies the SHA-256 of the whole image
 *   - rollback: the new firmware must reach the end of its boot and call
 *     fw_update_confirm_boot(); if it crashes before, the bootloader goes
 *     back to the previous firmware by itself
 *
 * Progress is published as STATE_UPDATE (phase) and STATE_UPDATE_PERCENT,
 * changed only when the whole percent changes: at most ~100 screen updates,
 * nothing that slows the transfer.
 */

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

typedef enum
{
    FW_UPDATE_IDLE,
    FW_UPDATE_CHECKING,     /* online: reading the manifest */
    FW_UPDATE_UP_TO_DATE,
    FW_UPDATE_AVAILABLE,    /* online: a newer version exists (fw_update_new_version) */
    FW_UPDATE_WRITING,      /* receiving / writing, see STATE_UPDATE_PERCENT */
    FW_UPDATE_RESTARTING,   /* done, restarting into the new firmware */
    FW_UPDATE_FAILED,       /* fw_update_message() says why */
} fw_update_phase_t;

/* Running firmware version (PROJECT_VER in the root CMakeLists.txt). */
const char *fw_update_running_version(void);

fw_update_phase_t fw_update_phase(void);
int fw_update_percent(void);

/* Error or status text for the last operation ("" if none). */
const char *fw_update_message(void);

/* Version found online (after FW_UPDATE_AVAILABLE). */
const char *fw_update_new_version(void);

/*
 * Streaming install, from any task, one at a time. `total_size` = bytes that
 * will follow. Each call returns an error to stop on; after an error the
 * update is already aborted.
 */
esp_err_t fw_update_begin(size_t total_size);
esp_err_t fw_update_write(const void *data, size_t length);
esp_err_t fw_update_end(void);              /* verify, select, restart in ~2 s */
void fw_update_abort(const char *reason);

/* Check the manifest (and install if `install`), in a background task. */
void fw_update_check_online(bool install);

/* Call when the boot finished well: the running firmware is kept for good. */
void fw_update_confirm_boot(void);

#endif
