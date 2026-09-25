add_executable(micro_sd
        main.cpp
        sd_spi.c
        sd_diskio.c
        file_browser.c
        usb_msc.c
        ssd1306.c
        )

target_include_directories(micro_sd PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../include
        )

# Los fuentes de TinyUSB se compilan dentro de este target (INTERFACE sources):
# necesita encontrar tusb_config.h (este directorio) y los headers del stack.
set(TINYUSB_SRC_DIR "${PICO_SDK_PATH}/lib/tinyusb/src")
if (EXISTS "${TINYUSB_SRC_DIR}")
    target_include_directories(micro_sd PRIVATE "${TINYUSB_SRC_DIR}")
else ()
    message(FATAL_ERROR "TinyUSB no encontrado en ${TINYUSB_SRC_DIR}: inicialice los submodulos del SDK")
endif ()

target_link_libraries(micro_sd PRIVATE
        pico_stdlib
        hardware_spi
        hardware_i2c
        fatfs
        tinyusb_device
        )

if (PICO_CYW43_SUPPORTED)
    # Pico W: LED onboard via CYW43
    target_link_libraries(micro_sd PRIVATE pico_cyw43_arch_none)
endif ()

# stdio por UART0 (GP0/GP1) para depuracion; USB queda reservado para MSC
pico_enable_stdio_usb(micro_sd 0)
pico_enable_stdio_uart(micro_sd 1)

pico_add_extra_outputs(micro_sd)
