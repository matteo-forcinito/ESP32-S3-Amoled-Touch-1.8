# Web Radio: external app for AMOLED Watch OS.
# Build, then copy build/webradio.bin to /sdcard/apps/WebRadio/WebRadio.bin
# (with icon.png and manifest.json from this folder), or upload it from the
# watch's web page.
cmake_minimum_required(VERSION 3.16)

# Shared watch OS: the repo's submodule, or the sibling folder before it exists.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../watch-os-common/components")
    set(WATCH_OS_COMMON "${CMAKE_CURRENT_LIST_DIR}/../../watch-os-common")
else()
    set(WATCH_OS_COMMON "${CMAKE_CURRENT_LIST_DIR}/../../../watch-os-common")
endif()
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components"   # this board's hardware
    "${WATCH_OS_COMMON}/components")                # shared watch OS
# Only the platform and the radio engine: not the base system's apps or Bluetooth.
set(EXCLUDE_COMPONENTS "apps" "companion" "hid_link")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(webradio)
