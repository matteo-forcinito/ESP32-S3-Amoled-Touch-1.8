set(DECK_SHARED "${WATCH_OS_COMMON}/deck/main")
idf_component_register(
    SRCS "${DECK_SHARED}/main.c" "${DECK_SHARED}/action.c" "${DECK_SHARED}/profiles.c" "${DECK_SHARED}/run.c" "${DECK_SHARED}/web.c" "${DECK_SHARED}/ui.c"
    INCLUDE_DIRS "${DECK_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash services hid_link cjson esp_wifi esp_netif esp_http_server
    EMBED_TXTFILES "${DECK_SHARED}/config.html"
)
