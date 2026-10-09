# AMOLED Watch OS - ESP-IDF firmware for the Waveshare ESP32-S3-Touch-AMOLED-1.8
# (and, through the board layer in components/hardware, its sister boards).
cmake_minimum_required(VERSION 3.16)

# Shared watch OS (core, ui, services, apps...): a sibling folder for now,
# a git submodule later. This project only adds its own hardware layer.
set(WATCH_OS_COMMON "${CMAKE_CURRENT_LIST_DIR}/../watch-os-common")
set(EXTRA_COMPONENT_DIRS "${WATCH_OS_COMMON}/components")

# The web radio engine is used by the external WebRadio app, not by the base system.
set(EXCLUDE_COMPONENTS "webradio" "hid_link")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Shown in Settings > Update; the online check compares it with the manifest.
set(PROJECT_VER "1.1.0")

project(amoled_watch)
