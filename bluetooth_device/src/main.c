/**
 * main.c - bluetooth_device entry point (Pico W / Pico 2 W)
 *
 * SPDX-License-Identifier: BSD-3-Clause (adapted from pico-examples bluetooth main.c)
 *
 * Initializes the CYW43439 WiFi+BT chip (cyw43) and hands control to the
 * BTstack run loop, which drives the A2DP/AVRCP source implemented in
 * a2dp_source_host.c (declared through btstack_main()).
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

/* Implemented in a2dp_source_host.c */
int btstack_main(int argc, const char * argv[]);

/* Used by the A2DP source to blink the onboard LED (cyw43 GPIO 0) */
void hal_led_toggle(void) {
    static int led_state;
    led_state = 1 - led_state;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
}

int main(void) {
    stdio_init_all();

    if (cyw43_arch_init() != PICO_OK) {
        panic("failed to init cyw43 (BT radio)");
    }

    printf("USB/Bluetooth host (A2DP source) starting...\n");

    /* Setup BTstack example (registers packet handlers + SDP records) */
    btstack_main(0, NULL);

    /* Run the BTstack run loop forever. */
    btstack_run_loop_execute();

    cyw43_arch_deinit();
    return 0;
}