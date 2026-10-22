set(MOTORE_SHARED "${WATCH_OS_COMMON}/motore/main")
idf_component_register(
    SRCS "${MOTORE_SHARED}/main.c" "${MOTORE_SHARED}/link.c" "${MOTORE_SHARED}/obd.c" "${MOTORE_SHARED}/dtc.c"
         "${MOTORE_SHARED}/ui.c" "${MOTORE_SHARED}/web.c"
    INCLUDE_DIRS "${MOTORE_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash appkit services bt lwip cjson esp_http_server
    EMBED_TXTFILES "${MOTORE_SHARED}/motore_dashboard.html"
)
