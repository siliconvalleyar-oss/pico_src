add_executable(midi
        main.c
        usb_descriptors.c
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(midi PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# Dependencies: pico_stdlib (core), hardware_adc (touch pad reading),
# tinyusb_device (USB), tinyusb_board (board support)
target_link_libraries(midi PUBLIC
        pico_stdlib
        hardware_adc
        tinyusb_device
        tinyusb_board)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(midi)
