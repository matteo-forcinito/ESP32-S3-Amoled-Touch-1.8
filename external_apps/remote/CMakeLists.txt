# Remote Control: external app for AMOLED Watch OS (Bluetooth / USB keyboard
# with swipe typing; mouse, media and more modes later).
# Build, then copy build/remote.bin to /sdcard/apps/Remote/Remote.bin
# (with icon.png and manifest.json from this folder), or upload it from the
# watch's web page.
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components"                  # this board's hardware
    "${CMAKE_CURRENT_LIST_DIR}/../../../watch-os-common/components") # shared watch OS
# The platform + hid_link: not the base system's apps, services or radio.
set(EXCLUDE_COMPONENTS "apps" "services" "companion" "webradio")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(remote)
