# One source for every board (watch-os-common/vault/main); the base OS never links it.
set(VAULT_SHARED "${WATCH_OS_COMMON}/vault/main")
set(SENTINEL_SHARED "${WATCH_OS_COMMON}/sentinel/main")
set(HID_SHARED "${WATCH_OS_COMMON}/components/hid_link")
idf_component_register(
    SRCS
        "${VAULT_SHARED}/main.c"
        "${VAULT_SHARED}/format.c"
        "${VAULT_SHARED}/crypto.c"
        "${VAULT_SHARED}/model.c"
        "${VAULT_SHARED}/store.c"
        "${VAULT_SHARED}/service.c"
        "${VAULT_SHARED}/ui.c"
        "${VAULT_SHARED}/input.c"
        "${VAULT_SHARED}/memory.c"
        "${VAULT_SHARED}/web.c"
        "${VAULT_SHARED}/hid.c"
        "${VAULT_SHARED}/ble.c"
        "${VAULT_SHARED}/bonds.c"
        "${VAULT_SHARED}/alarm.c"
        "${SENTINEL_SHARED}/packet.c"
        "${HID_SHARED}/hid_link.c"
        "${HID_SHARED}/hid_usb.c"
        "${HID_SHARED}/hid_keymap.c"
        "${WATCH_OS_COMMON}/components/apps/alarms/ring_app.c"
    INCLUDE_DIRS "${VAULT_SHARED}"
    PRIV_INCLUDE_DIRS "${SENTINEL_SHARED}" "${HID_SHARED}" "${HID_SHARED}/include"
                      "${WATCH_OS_COMMON}/components/apps"
    PRIV_REQUIRES hardware core ui services extapp_sdk mbedtls bootloader_support esp_timer
                  esp_wifi esp_netif lwip cjson esp_https_server bt esp_hid esp_tinyusb nvs_flash
    EMBED_TXTFILES "${VAULT_SHARED}/dashboard.html" "${VAULT_SHARED}/dashboard.js"
)
# Vault-only hooks: wipe freed LVGL text buffers, private BLE bond namespace (memory.c, bonds.c).
# -u keeps memory.c in the link: nothing in main calls the wrappers directly.
target_link_options(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=lv_free_core" "-Wl,--wrap=lv_realloc_core"
                    "-Wl,--wrap=nvs_open" "-Wl,-u,__wrap_lv_free_core" "-Wl,-u,__wrap_lv_realloc_core")
# QA builds (VAULT_QA=1 in the environment) create synthetic data in <sd>/vault-qa only.
if(DEFINED ENV{VAULT_QA})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE VAULT_QA=1)
endif()
