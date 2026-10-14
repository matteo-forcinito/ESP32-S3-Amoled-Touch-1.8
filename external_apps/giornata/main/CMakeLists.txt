set(GIORNATA_SHARED "${WATCH_OS_COMMON}/giornata/main")
idf_component_register(
    SRCS "${GIORNATA_SHARED}/main.c" "${GIORNATA_SHARED}/ics.c" "${GIORNATA_SHARED}/store.c" "${GIORNATA_SHARED}/sync.c" "${GIORNATA_SHARED}/agenda.c" "${GIORNATA_SHARED}/ui.c" "${GIORNATA_SHARED}/web.c"
    INCLUDE_DIRS "${GIORNATA_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash appkit services esp_http_client esp_http_server mbedtls cjson
    EMBED_TXTFILES "${GIORNATA_SHARED}/giornata_dashboard.html" "${GIORNATA_SHARED}/gts_root_r1.pem"
)
