add_executable(adc_oled
        main.c
        usb_descriptors.c
        )

# Make sure the source directory is in the include path
target_include_directories(adc_oled PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# Dependencies: pico_stdlib (core), hardware_adc (ADC reading),
# hardware_i2c (OLED communication), tinyusb_device (USB CDC serial),
# tinyusb_board (board support for USB)
target_link_libraries(adc_oled PUBLIC
        pico_stdlib
        hardware_adc
        hardware_i2c
        tinyusb_device
        tinyusb_board)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(adc_oled)
