idf_component_register(
    SRCS "main.c" "remote.c" "remote_home.c" "remote_options.c" "keyboard_app.c" "swipe.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES hardware core ui hid_link extapp_sdk nvs_flash
    EMBED_TXTFILES "dict_it.txt" "dict_en.txt"
)
