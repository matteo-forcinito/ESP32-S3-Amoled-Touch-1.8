idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES hardware core ui services apps nvs_flash app_update
)
