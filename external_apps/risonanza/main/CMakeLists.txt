set(RISONANZA_SHARED "${WATCH_OS_COMMON}/risonanza/main")
idf_component_register(
    SRCS "${RISONANZA_SHARED}/main.c" "${RISONANZA_SHARED}/install.c" "${RISONANZA_SHARED}/dsp.c" "${RISONANZA_SHARED}/measure.c" "${RISONANZA_SHARED}/plate.c" "${RISONANZA_SHARED}/store.c"
    INCLUDE_DIRS "${RISONANZA_SHARED}"
    PRIV_REQUIRES hardware core ui extapp_sdk esp_timer nvs_flash app_update bootloader_support lvgl
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -O2)
# Microphone capture and the ~112 Hz IMU profile apply only to Risonanza, never the base OS.
idf_component_get_property(RISONANZA_HARDWARE_LIB hardware COMPONENT_LIB)
target_compile_definitions(${RISONANZA_HARDWARE_LIB} PRIVATE CONFIG_WATCH_VOICE_MEMOS=1)
idf_component_get_property(RISONANZA_SENSOR_LIB board_drivers COMPONENT_LIB)
target_compile_definitions(${RISONANZA_SENSOR_LIB} PRIVATE WATCH_QMI8658_ODR=6 WATCH_QMI8658_RECOVER_SAME_PROFILE=1)
