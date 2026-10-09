# Web Radio: external app for AMOLED Watch OS.
# Build, then copy build/webradio.bin to /sdcard/apps/WebRadio/WebRadio.bin
# (with icon.png and manifest.json from this folder), or upload it from the
# watch's web page.
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components")
# Only the platform and the radio engine: not the base system's apps or Bluetooth.
set(EXCLUDE_COMPONENTS "apps" "companion" "hid_link")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(webradio)
