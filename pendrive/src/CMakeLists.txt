add_executable(pendrive
        main.c
        usb_descriptors.c
        msc_disk.c
        )

# Make sure the source directory is in the include path
target_include_directories(pendrive PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# Dependencies: pico_stdlib (core), tinyusb_device (USB device stack),
# tinyusb_board (board support for USB). The tinyusb MSC class is enabled
# through tusb_config.h (CFG_TUD_MSC).
target_link_libraries(pendrive PUBLIC
        pico_stdlib
        tinyusb_device
        tinyusb_board)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(pendrive)
