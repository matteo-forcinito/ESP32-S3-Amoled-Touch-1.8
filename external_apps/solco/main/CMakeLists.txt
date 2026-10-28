set(SOLCO_SHARED "${WATCH_OS_COMMON}/solco/main")
idf_component_register(
    SRCS "${SOLCO_SHARED}/main.c" "${SOLCO_SHARED}/install.c" "${SOLCO_SHARED}/engine.c" "${SOLCO_SHARED}/mix.c" "${SOLCO_SHARED}/vinyl.c" "${SOLCO_SHARED}/keylock.c" "${SOLCO_SHARED}/mp3.c" "${SOLCO_SHARED}/prepare.c" "${SOLCO_SHARED}/wav.c" "${SOLCO_SHARED}/studio.c" "${SOLCO_SHARED}/studio_motion.c" "${SOLCO_SHARED}/studio_clock.c" "${SOLCO_SHARED}/ui_kit.c" "${SOLCO_SHARED}/ui_decks.c" "${SOLCO_SHARED}/ui_master.c" "${SOLCO_SHARED}/ui_studio.c" "${SOLCO_SHARED}/link.c"
    INCLUDE_DIRS "${SOLCO_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash app_update bootloader_support mbedtls fatfs esp_wifi esp_event
    EMBED_FILES "${SOLCO_SHARED}/../assets/pulse.pcm" "${SOLCO_SHARED}/../assets/chord.pcm" "${SOLCO_SHARED}/../assets/import-test.mp3"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -O2)
# The DMA queue is chosen at run time through audio_start_ex (latency profiles).

target_compile_definitions(${COMPONENT_LIB} PRIVATE SOLCO_OUTPUT_CHANNELS=1 SOLCO_LINK=1)

idf_component_get_property(SOLCO_SENSOR_LIB board_drivers COMPONENT_LIB)
target_compile_definitions(${SOLCO_SENSOR_LIB} PRIVATE WATCH_QMI8658_ODR=6 WATCH_QMI8658_RECOVER_SAME_PROFILE=1)
