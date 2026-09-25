cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(micro_sd C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# FatFS (Elm-chan) como librería estática; diskio vive en src/ (sd_diskio.c)
add_subdirectory(lib/fatfs)

# Firmware principal
add_subdirectory(src)
