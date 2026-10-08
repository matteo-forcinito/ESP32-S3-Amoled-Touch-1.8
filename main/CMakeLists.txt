idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES hardware core ui services companion apps nvs_flash app_update
)
