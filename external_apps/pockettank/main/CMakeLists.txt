# Mirrors pockettank/firmware/main upstream, built hosted (PT_WATCH_OS).
set(PT_MAIN "${PT_DIR}/firmware/main")
set(SRCS main.c director.c brightness.c batlog.c codec_port.c advisor_llm_esp.c battery_port_axp2101.c imu_port_qmi8658.c
         update_mode.c net_port_esp.c model_trailer.c pt_watch_os.c
         display_port_sh8601.c touch_port_ft3168.c audio_port_es8311.c)
list(TRANSFORM SRCS PREPEND "${PT_MAIN}/")
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "${PT_MAIN}"
    REQUIRES tankcore llm extapp_sdk esp_partition spi_flash esp_timer esp_driver_spi esp_driver_i2c esp_driver_i2s esp_driver_gpio
             esp_driver_usb_serial_jtag esp_lcd nvs_flash esp_app_format esp_wifi esp_netif esp_event esp_http_client
             esp_https_ota app_update mbedtls bootloader_support
    EMBED_FILES "${PT_MAIN}/tokenizer.bin" "${PT_DIR}/assets/sounds/sounds.bin"
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE PT_WATCH_OS=1)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u pt_board_marker")
