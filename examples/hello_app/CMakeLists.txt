# Example external app: build it, copy build/hello_app.bin to
# /sdcard/apps/Hello/Hello.bin and open it from "App esterne".
cmake_minimum_required(VERSION 3.16)

# Reuse the whole watch platform: board, LVGL port, theme, widgets...
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components"                  # this board's hardware
    "${CMAKE_CURRENT_LIST_DIR}/../../../watch-os-common/components") # shared watch OS
# ...but not the parts this app does not need.
set(EXCLUDE_COMPONENTS "apps" "services" "companion" "webradio" "hid_link")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hello_app)
