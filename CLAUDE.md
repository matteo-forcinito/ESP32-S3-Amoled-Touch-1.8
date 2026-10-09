# CLAUDE.md — project summary

Smartwatch-style firmware for Waveshare ESP32-S3-Touch-AMOLED-1.8 (SH8601 368x448
QSPI, FT3168, AXP2101, PCF85063, ES8311, microSD, 16 MB flash, 8 MB PSRAM).
ESP-IDF 6.1, C, LVGL 9.6. Code/comments in English, Allman braces, 4 spaces,
snake_case, `s_` statics, `TAG` per file, beginner-friendly header comments.
Human docs: README.md (Italian). Old Arduino code: `Launcher/`, `External APPS/` (reference only).

## Shared OS (watch-os-common submodule)

core, ui, services, apps, companion, extapp_sdk, webradio, hid_link and board_api live in
`watch-os-common/components` (EXTRA_COMPONENT_DIRS), shared with the SenseCAP Watcher
firmware (`../SenseCap-Watcher-W1-A`). This repo keeps only `components/hardware` + main/apps.
- `board_api` = the hardware headers (display.h, touch.h, pmu.h, knob.h...). Each firmware's
  `hardware` component implements them; shared code never sees pins or chip names.
- A change in watch-os-common must build in BOTH firmwares (round 412x412 + knob vs 368x448).
- watch-os-common is a git submodule (./watch-os-common, repo matteo-forcinito/watch-os-common).
  Edit the library in the sibling checkout ../watch-os-common and build with
  `$env:WATCH_OS_COMMON_DIR="C:/Users/matte/Documents/ESP-IDF/watch-os-common"`; then commit+push it
  and run `git submodule update --remote watch-os-common` here (commit the new pointer).
- Every build also writes build/firmware.bin (same name on all devices; release asset too). The
  project name stays the image identity checked by the updater: never rename it.
- Releases: push a branch release/X.Y.Z (stable) or release/X.Y.Z-dev.N (dev); see docs/RELEASE.md.
- Board-specific behaviour goes behind board_api (e.g. display_round_area()) or board_info() flags
  (round, has_knob, has_usb_otg, product/short_name for names shown to users).
- Round panels: use ui_page()/ui_title()/ui_text() (they centre on round boards); never left-align
  text near the top. Knob focus: custom clickable objects need lv_port_set_focusable(obj).
- lv_port_refresh_now() renders in the lvgl task (never render in a caller's small stack).

## Build

```powershell
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
idf.py build                      # -Werror on our code, keep it warning-free
cd examples/hello_app; idf.py build
cd external_apps/webradio; idf.py build   # the web radio is an EXTERNAL app
cd external_apps/remote; idf.py build     # Remote Control (BLE/USB HID keyboard, swipe typing)
```

Base system = small OS (alarms, notifications, BLE companion, settings, web page,
USB drive, external app launcher). Heavy features go to external apps that reuse
the components (EXTRA_COMPONENT_DIRS + EXCLUDE_COMPONENTS). `webradio` and `hid_link` are
excluded from the base build in the root CMakeLists.

## Layers (each uses only those below)

apps (shell + apps) -> companion / webradio / hid_link -> services -> ui -> core -> hardware. Board pins/flags only in
`components/hardware/boards/*.h`; above hardware use `board_info()`. Shared layers are in ../watch-os-common.

## Rules that prevent bugs

- LVGL only in the `lvgl` task. Other tasks: `ui_async()` or `lv_port_lock/unlock`.
  Never block in UI callbacks (network, radio_player_stop): spawn a task.
- Services publish with `state_set/state_bump` (never lock LVGL). UI observes with
  `lv_subject_add_observer_obj(state_subject(X), cb, obj, user)` (auto-removed with obj).
- esp_timer callbacks never do real work: `sys_post()` it to the sys worker
  (NVS writes, esp_wifi_stop, alarm checks, BLE sends). Its stack is 4 KB.
- Never lv_obj_delete() a screen: the app manager retires screens and deletes
  them when LVGL no longer refers to them; app destroy() runs at that moment.
  Args passed to app_open*() must stay valid (the open may be queued): pass
  malloc'd data and free it in create().
- Services read by the UI at creation must be initialized before `apps_init()`
  (see main.c / apps.c order); public getters should not assume init happened.
- LVGL 9.6: `lv_obj_add_flag/remove_flag` are deprecated -> `lv_obj_set_clickable()` etc.;
  subjects via `lv_subject_create()`.
- Per-instance UI state (widgets used in more than one place) must not be static:
  allocate and free on LV_EVENT_DELETE (see shell/now_playing.c).
- THE ALARM MUST ALWAYS RING: the sound service retries the speaker every second while
  an alarm rings, ring_app restarts the melody if it stopped. Keep it.
- UI code never calls blocking service functions (ble_companion_enable, radio_player_stop,
  wifi_acquire): save the setting / post a command / start a task.
- Crash? Boot log line `sys: PREVIOUS RUN CRASHED` + `idf.py coredump-info`.
- Wi-Fi is reference counted: every `wifi_acquire()` needs a `wifi_release()`.
- Power: hold `power_cpu_boost()` only while really computing; `power_keep_screen_on()` balanced.
- Settings: append fields at the END of `settings_t` with a default in settings.c.
- Internal RAM is the scarce resource: LVGL allocates in PSRAM (core/lv_mem_psram.c,
  CONFIG_LV_USE_CUSTOM_MALLOC); start tasks with `sys_task_create()` (logs failures);
  big static arrays -> `EXT_RAM_BSS_ATTR`. Check the `sys: [...]` heap lines at boot.
- Pins that must hold state in light sleep: add them in board.c keep_pins_in_light_sleep().
- Text fonts are built-in Montserrat + TTF fallback for accents: always use UI_FONT_*.
- Edit scripts on Windows: never pass "\0" through bash heredocs (becomes a NUL byte);
  write a .py file instead.
- Remote Control: swipe decoder `external_apps/remote/main/swipe.c` is pure C (host-testable);
  its layout must match the key geometry drawn in keyboard_app.c. Changing BLE<->USB
  restarts the app (`extapp_restart_self()`): HID stacks are never torn down at runtime.
- Firmware update: services/update/fw_update.c (web POST /api/firmware, online manifest
  settings.update_url). Rollback is on: main.c calls fw_update_confirm_boot() at the END
  of the boot - keep it last. Bump PROJECT_VER in the root CMakeLists for releases.
- USB cable power (PMU VBUS, polled with the PWR key) holds a no-light-sleep lock in
  power.c and re-announces the USB console (hardware/usb_console.c): keep it.
- New .c files go into the component's CMakeLists SRCS; new apps into apps.c `s_apps[]`.
