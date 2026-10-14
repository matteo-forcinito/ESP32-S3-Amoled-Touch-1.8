set(PIENO_SHARED "${WATCH_OS_COMMON}/pieno/main")
idf_component_register(
    SRCS "${PIENO_SHARED}/main.c" "${PIENO_SHARED}/feed.c" "${PIENO_SHARED}/fuel.c" "${PIENO_SHARED}/ui.c" "${PIENO_SHARED}/web.c"
    INCLUDE_DIRS "${PIENO_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash appkit services esp_http_client esp_http_server mbedtls cjson
    EMBED_TXTFILES "${PIENO_SHARED}/dashboard.html"
)
