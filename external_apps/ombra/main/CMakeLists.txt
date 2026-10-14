set(OMBRA_SHARED "${WATCH_OS_COMMON}/ombra/main")
idf_component_register(
    SRCS "${OMBRA_SHARED}/main.c" "${OMBRA_SHARED}/scanner.c" "${OMBRA_SHARED}/classify.c" "${OMBRA_SHARED}/sound.c" "${OMBRA_SHARED}/ui.c"
    INCLUDE_DIRS "${OMBRA_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash bt appkit services
)
