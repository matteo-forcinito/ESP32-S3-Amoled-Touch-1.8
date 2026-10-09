# AMOLED Watch OS - ESP-IDF firmware for the Waveshare ESP32-S3-Touch-AMOLED-1.8
# (and, through the board layer in components/hardware, its sister boards).
cmake_minimum_required(VERSION 3.16)

# Shared watch OS (core, ui, services, apps...): the git submodule
# ./watch-os-common, or (before the submodule is set up) the sibling folder.
# This project only adds its own hardware layer.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/watch-os-common/components")
    set(WATCH_OS_COMMON "${CMAKE_CURRENT_LIST_DIR}/watch-os-common")
else()
    set(WATCH_OS_COMMON "${CMAKE_CURRENT_LIST_DIR}/../watch-os-common")
endif()
set(EXTRA_COMPONENT_DIRS "${WATCH_OS_COMMON}/components")

# The web radio engine is used by the external WebRadio app, not by the base system.
set(EXCLUDE_COMPONENTS "webradio" "hid_link")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Shown in Settings > Update; the online check compares it with the manifest.
# Release builds get the version from the CI (branch release/X.Y.Z -> FW_VERSION).
if(DEFINED ENV{FW_VERSION} AND NOT "$ENV{FW_VERSION}" STREQUAL "")
    set(PROJECT_VER "$ENV{FW_VERSION}")
else()
    set(PROJECT_VER "1.1.0")
endif()

project(amoled_watch)
