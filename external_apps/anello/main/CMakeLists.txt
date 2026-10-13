set(ANELLO_SHARED "${WATCH_OS_COMMON}/anello/main")
idf_component_register(
    SRCS "${ANELLO_SHARED}/main.c" "${ANELLO_SHARED}/game.c"
    INCLUDE_DIRS "${ANELLO_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash
)
