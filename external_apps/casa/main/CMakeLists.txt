set(CASA_SHARED "${WATCH_OS_COMMON}/casa/main")
idf_component_register(
    SRCS "${CASA_SHARED}/main.c" "${CASA_SHARED}/ha.c" "${CASA_SHARED}/web.c" "${CASA_SHARED}/ui.c"
    INCLUDE_DIRS "${CASA_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash services cjson esp_wifi esp_netif esp_http_server mbedtls espressif__esp_websocket_client espressif__mdns
    EMBED_TXTFILES "${CASA_SHARED}/setup.html"
)
