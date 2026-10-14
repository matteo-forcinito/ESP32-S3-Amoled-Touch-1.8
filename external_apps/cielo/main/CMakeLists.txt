set(CIELO_SHARED "${WATCH_OS_COMMON}/cielo/main")
idf_component_register(
    SRCS "${CIELO_SHARED}/main.c" "${CIELO_SHARED}/feed.c" "${CIELO_SHARED}/types.c" "${CIELO_SHARED}/ui.c"
    INCLUDE_DIRS "${CIELO_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash services cjson esp_wifi esp_netif esp_http_client appkit
)
