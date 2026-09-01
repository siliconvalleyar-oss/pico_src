cmake_minimum_required(VERSION 3.13)

# Point to the Raspberry Pi Pico SDK (2 levels up from this project dir).
if (NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../pico-sdk")
endif ()

# Must be included before project()
include(pico_sdk_import.cmake)

project(pico_usb_drive_configurable C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise the Pico SDK
pico_sdk_init()

# Vendor FatFS (Elm-chan) as a reusable static library
add_subdirectory(lib/fatfs)

# Main application (uses src/ for the firmware)
add_subdirectory(src)
