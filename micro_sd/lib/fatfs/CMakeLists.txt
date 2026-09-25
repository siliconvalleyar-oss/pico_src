# FatFS (Elm-chan R0.15a) - nucleo del filesystem.
# Las funciones disk_* (diskio) se implementan en src/sd_diskio.c contra la microSD.

add_library(fatfs STATIC
        source/ff.c
        source/ffsystem.c
        source/ffunicode.c
        )

set_target_properties(fatfs PROPERTIES C_STANDARD 11)

target_include_directories(fatfs PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/source
        )

target_link_libraries(fatfs PUBLIC pico_stdlib)
