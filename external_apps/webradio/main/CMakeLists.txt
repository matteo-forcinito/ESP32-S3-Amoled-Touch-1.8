idf_component_register(
    SRCS "main.c" "radio_ui.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES hardware core ui services webradio extapp_sdk
)
