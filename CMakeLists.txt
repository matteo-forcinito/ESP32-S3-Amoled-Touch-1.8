# AMOLED Watch OS - ESP-IDF firmware for the Waveshare ESP32-S3-Touch-AMOLED-1.8
# (and, through the board layer in components/hardware, its sister boards).
cmake_minimum_required(VERSION 3.16)

# The web radio engine is used by the external WebRadio app, not by the base system.
set(EXCLUDE_COMPONENTS "webradio" "hid_link")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(amoled_watch)
