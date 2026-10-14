set(COPERTURA_SHARED "${WATCH_OS_COMMON}/copertura/main")
idf_component_register(
    SRCS "${COPERTURA_SHARED}/main.c" "${COPERTURA_SHARED}/advice.c" "${COPERTURA_SHARED}/probe.c" "${COPERTURA_SHARED}/store.c" "${COPERTURA_SHARED}/sound.c" "${COPERTURA_SHARED}/ui.c" "${COPERTURA_SHARED}/web.c"
    INCLUDE_DIRS "${COPERTURA_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash appkit services esp_http_server esp_wifi esp_netif lwip cjson
    EMBED_TXTFILES "${COPERTURA_SHARED}/dashboard.html"
)
