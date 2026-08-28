cmake_minimum_required(VERSION 3.13)

# Set PICO_SDK_PATH to the SDK directory (2 levels up from midi/)
set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../pico-sdk")

# Pull in Pico SDK (must be done before project())
include(pico_sdk_import.cmake)

project(midi_touch_pads C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Add source subdirectory
add_subdirectory(src)
