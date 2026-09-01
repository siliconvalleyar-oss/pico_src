/*
 * pendrive - Raspberry Pi Pico RP2040 acting as a USB pendrive
 *
 * Exposes a configurable, writable FAT disk over USB Mass Storage (MSC).
 * The disk contents live in the Pico's SRAM and are formatted at boot
 * (see msc_disk.h / msc_disk.c).
 *
 * Also provides a USB CDC virtual serial port for a few diagnostics.
 * Commands over CDC:
 *   INFO   -> show disk geometry and state
 *   WITHOUT -> no-op placeholder (kept for symmetry with other projects)
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "msc_disk.h"

//====================================================================+
// CONFIGURATION
//====================================================================+

// Blink pattern: 250ms = not mounted, 1000ms = mounted, 2500ms = suspended
enum {
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED     = 1000,
    BLINK_SUSPENDED   = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

//====================================================================+
// CDC HELPERS
//====================================================================+

static void cdc_init(void) {
    tusb_rhport_init_t dev_init = {
        .role  = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);
}

static void cdc_send_string(const char *str) {
    if (!tud_cdc_connected()) return;
    tud_cdc_write(str, strlen(str));
    tud_cdc_write_flush();
}

static void process_serial_command(const char *cmd) {
    if (strcmp(cmd, "INFO") == 0) {
        char resp[128];
        snprintf(resp, sizeof(resp),
            "[INFO] blocks=%lu size=%lu | fs=%s | ejected=%s | mounted=%s\r\n",
            (unsigned long) pendisk_block_count(),
            (unsigned long) (pendisk_block_count() * 512u / 1024u),
            "FAT", pendisk_is_ejected() ? "yes" : "no",
            tud_mounted() ? "yes" : "no");
        cdc_send_string(resp);
    } else {
        cdc_send_string("[INFO] unknown command (try INFO)\r\n");
    }
}

static void check_serial_commands(void) {
    if (!tud_cdc_connected()) return;

    static char cmd_buf[64];
    static uint8_t cmd_len = 0;

    while (tud_cdc_available()) {
        char c = tud_cdc_read_char();
        if (c == '\r' || c == '\n') {
            if (cmd_len > 0) {
                cmd_buf[cmd_len] = '\0';
                process_serial_command(cmd_buf);
                cmd_len = 0;
            }
        } else if (cmd_len < sizeof(cmd_buf) - 1) {
            cmd_buf[cmd_len++] = c;
        }
    }
}

//====================================================================+
// LED TASK
//====================================================================+

static void led_blinking_task(void) {
    static uint32_t start_ms = 0;
    static bool led_state = false;

    if (board_millis() - start_ms < blink_interval_ms) return;
    start_ms += blink_interval_ms;

    gpio_put(PICO_DEFAULT_LED_PIN, led_state);
    led_state = 1 - led_state;
}

//====================================================================+
// MAIN
//====================================================================+

int main(void) {
    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    // Format the RAM disk ONCE before USB enumeration.
    pendisk_format();

    cdc_init();
    sleep_ms(500);

    cdc_send_string("\r\n");
    cdc_send_string("========================================\r\n");
    cdc_send_string("  Pico Pendrive\r\n");
    cdc_send_string("  Raspberry Pi Pico RP2040\r\n");
    cdc_send_string("========================================\r\n");
    cdc_send_string("[SYS] RAM disk formatted\r\n");

    char info[80];
    snprintf(info, sizeof(info),
        "[SYS] Capacity: %lu KB (%lu x 512B blocks)\r\n",
        (unsigned long) (pendisk_block_count() * 512u / 1024u),
        (unsigned long) pendisk_block_count());
    cdc_send_string(info);
    cdc_send_string("[SYS] Ready. Connect USB to mount the pendrive.\r\n");

    while (1) {
        tud_task();           // tinyusb device task
        led_blinking_task();
        check_serial_commands();
    }

    return 0;
}

//====================================================================+
// USB DEVICE CALLBACKS
//====================================================================+

void tud_mount_cb(void) {
    blink_interval_ms = BLINK_MOUNTED;
    cdc_send_string("[USB] Device mounted\r\n");
}

void tud_umount_cb(void) {
    blink_interval_ms = BLINK_NOT_MOUNTED;
    cdc_send_string("[USB] Device unmounted\r\n");
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    blink_interval_ms = BLINK_SUSPENDED;
    cdc_send_string("[USB] Device suspended\r\n");
}

void tud_resume_cb(void) {
    blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
    cdc_send_string("[USB] Device resumed\r\n");
}
