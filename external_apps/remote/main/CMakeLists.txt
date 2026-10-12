# One source for every board; the base OS never links this external app.
set(REMOTE_SHARED "${WATCH_OS_COMMON}/remote/main")
idf_component_register(
    SRCS
        "${REMOTE_SHARED}/main.c"
        "${REMOTE_SHARED}/remote.c"
        "${REMOTE_SHARED}/remote_home.c"
        "${REMOTE_SHARED}/remote_options.c"
        "${REMOTE_SHARED}/keyboard_app.c"
        "${REMOTE_SHARED}/pc_keyboard.c"
        "${REMOTE_SHARED}/swipe.c"
        "${REMOTE_SHARED}/remote_modes.c"
        "${REMOTE_SHARED}/remote_alarm.c"
        "${WATCH_OS_COMMON}/components/apps/alarms/ring_app.c"
    INCLUDE_DIRS "${REMOTE_SHARED}"
    PRIV_INCLUDE_DIRS "${WATCH_OS_COMMON}/components/apps"
    PRIV_REQUIRES hardware core ui services hid_link extapp_sdk nvs_flash
    EMBED_TXTFILES "${REMOTE_SHARED}/dict_it.txt" "${REMOTE_SHARED}/dict_en.txt"
)
