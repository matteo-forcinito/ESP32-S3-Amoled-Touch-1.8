set(ORBIT_SHARED "${WATCH_OS_COMMON}/orbit/main")
idf_component_register(
    SRCS "${ORBIT_SHARED}/main.c" "${ORBIT_SHARED}/orbit.c" "${ORBIT_SHARED}/orbit_alarm.c"
         "${WATCH_OS_COMMON}/components/apps/alarms/ring_app.c"
    INCLUDE_DIRS "${ORBIT_SHARED}"
    PRIV_INCLUDE_DIRS "${WATCH_OS_COMMON}/components/apps"
    PRIV_REQUIRES hardware core ui services extapp_sdk esp_timer nvs_flash
)
