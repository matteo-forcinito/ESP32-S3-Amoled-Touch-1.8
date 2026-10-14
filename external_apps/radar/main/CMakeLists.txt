set(RADAR_SHARED "${WATCH_OS_COMMON}/radar/main")
idf_component_register(
    SRCS "${RADAR_SHARED}/main.c" "${RADAR_SHARED}/feed.c" "${RADAR_SHARED}/compose.c" "${RADAR_SHARED}/ui.c"
    INCLUDE_DIRS "${RADAR_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash services cjson esp_wifi esp_netif esp_http_client mbedtls appkit
)
