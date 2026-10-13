set(SENTINEL_SHARED "${WATCH_OS_COMMON}/sentinel/main")
if(DEFINED ENV{SENTINEL_SOURCE_DIR})
    set(SENTINEL_SHARED "$ENV{SENTINEL_SOURCE_DIR}")
endif()
idf_component_register(
    SRCS "${SENTINEL_SHARED}/main.c" "${SENTINEL_SHARED}/probe.c" "${SENTINEL_SHARED}/store.c"
         "${SENTINEL_SHARED}/packet.c" "${SENTINEL_SHARED}/web.c" "${SENTINEL_SHARED}/ui.c"
         "${SENTINEL_SHARED}/alarm.c" "${WATCH_OS_COMMON}/components/apps/alarms/ring_app.c"
    INCLUDE_DIRS "${SENTINEL_SHARED}"
    PRIV_INCLUDE_DIRS "${WATCH_OS_COMMON}/components/apps"
    PRIV_REQUIRES hardware core ui services extapp_sdk esp_timer esp_wifi esp_netif lwip cjson
                  esp_http_server mdns mbedtls
    EMBED_TXTFILES "${SENTINEL_SHARED}/dashboard.html"
)
target_link_options(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=esp_netif_receive")
