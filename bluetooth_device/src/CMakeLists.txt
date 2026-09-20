# bluetooth_device - Bluetooth Audio Host (A2DP Source) for Pico W
#
# Links the official BTstack libraries from the pico-sdk:
#   - pico_btstack_cyw43     : CYW43439 BT radio driver (Pico W / Pico 2 W)
#   - pico_btstack_classic   : Classic Bluetooth (BR/EDR) for A2DP
#   - pico_btstack_ble       : BLE support
#   - pico_btstack_sbc_encoder : SBC audio encoder for A2DP
#   - pico_btstack_hxcmod_player : ProTracker MOD player (audio source)

add_executable(bluetooth_device
        main.c
        a2dp_source_host.c
        ../mods/nao-deceased_by_disease.c
        )

# The HCI MOD player + MOD assets
target_include_directories(bluetooth_device PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../mods
        ${CMAKE_CURRENT_LIST_DIR}/..
        )

# Include the BTstack config (btstack_config.h is at the project root)
target_include_directories(bluetooth_device PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/..
        )

# Supplier the hxcmod player interface library providing hxcmod symbols.
# The pico-sdk ships the mod sources; we compile the mod data separately
# (../mods/nao-deceased_by_disease.c) and link the player core + symbols.
add_library(bluetooth_hxcmod INTERFACE)
target_sources(bluetooth_hxcmod INTERFACE
        ${PICO_SDK_PATH}/lib/btstack/3rd-party/hxcmod-player/hxcmod.c
        )
target_include_directories(bluetooth_hxcmod INTERFACE
        ${PICO_SDK_PATH}/lib/btstack/3rd-party/hxcmod-player
        ${PICO_SDK_PATH}/lib/btstack/3rd-party/hxcmod-player/mods
        )

target_link_libraries(bluetooth_device PRIVATE
        pico_stdlib
        pico_cyw43_arch_none          # cyw43 without lwIP (we don't need WiFi)
        pico_stdio_usb                # USB CDC serial for logs
        pico_btstack_cyw43            # BT radio on CYW43439
        pico_btstack_classic          # Classic BT (A2DP source)
        pico_btstack_ble              # BLE (dual-mode)
        pico_btstack_sbc_encoder      # SBC audio encoder
        bluetooth_hxcmod              # MOD player core
        )

# Custom USB identity (shown in lsusb / /dev/serial/by-id)
target_compile_definitions(bluetooth_device PRIVATE
        USBD_MANUFACTURER="siliconvalleyar"
        USBD_PRODUCT="Pico W BT Host"
        PICO_STDIO_USB_CONNECT_WITHOUT_DETECTION=1)

# Enable extra outputs (uf2, bin, map, ...)
pico_add_extra_outputs(bluetooth_device)