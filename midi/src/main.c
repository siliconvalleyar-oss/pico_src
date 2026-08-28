/*
 * MIDI Touch Pads - Raspberry Pi Pico USB MIDI Controller
 *
 * Reads analog touch pads on ADC pins (GPIO 26, 27, 28) and converts
 * touch/no-touch into MIDI Note On/Off messages sent over USB.
 *
 * Hardware:
 *   - Touch pad on GPIO 26 (ADC0) -> MIDI Note C4 (60)
 *   - Touch pad on GPIO 27 (ADC1) -> MIDI Note D4 (62)
 *   - Touch pad on GPIO 28 (ADC2) -> MIDI Note E4 (64)
 *   - Optional: internal temperature sensor as pad 4 -> MIDI Note G4 (67)
 *
 * Touch detection: Each pad reads its ADC voltage. When a finger touches
 * the pad (conductive to GND through the body), the voltage drops below
 * the threshold. You can also use capacitive touch circuits.
 *
 * Velocity is derived from the ADC value (lower voltage = harder touch = louder).
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

//--------------------------------------------------------------------+
// Configuration
//--------------------------------------------------------------------+

// Number of touch pad channels
#define NUM_PADS        4

// ADC pins for touch pads (GPIO 26=ADC0, 27=ADC1, 28=ADC2)
// Pad 3 uses the internal temperature sensor (ADC4)
static const uint8_t adc_pins[NUM_PADS] = { 26, 27, 28 };

// MIDI note numbers mapped to each pad
// Using notes from C major scale: C4, D4, E4, G4
static const uint8_t midi_notes[NUM_PADS] = { 60, 62, 64, 67 };

// ADC threshold: values below this are considered "touched"
// ADC is 12-bit (0-4095). Adjust based on your touch pad circuit.
// Typical resting value is ~2048 (mid-rail), touch pulls toward 0.
#define TOUCH_THRESHOLD     1500

// Minimum velocity (so even light touches are audible)
#define MIN_VELOCITY        30

// Maximum velocity
#define MAX_VELOCITY        127

// Debounce: how many consecutive readings before changing state
#define DEBOUNCE_COUNT      3

// Scan interval in milliseconds
#define SCAN_INTERVAL_MS    10

//--------------------------------------------------------------------+
// Blink pattern
//--------------------------------------------------------------------+

enum {
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

//--------------------------------------------------------------------+
// Touch pad state
//--------------------------------------------------------------------+

typedef struct {
    bool     is_touched;      // current debounced state
    uint8_t  debounce_cnt;    // debounce counter
    uint8_t  velocity;        // MIDI velocity
    uint16_t raw_value;       // last raw ADC value
} pad_state_t;

static pad_state_t pads[NUM_PADS];

//--------------------------------------------------------------------+
// Prototypes
//--------------------------------------------------------------------+

void led_blinking_task(void);
void midi_task(void);
void adc_init_pads(void);
void scan_pads(void);
void send_midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity);
void send_midi_note_off(uint8_t channel, uint8_t note);
void send_midi_cc(uint8_t channel, uint8_t cc, uint8_t value);

//--------------------------------------------------------------------+
// ADC initialization
//--------------------------------------------------------------------+

void adc_init_pads(void) {
    adc_init();

    // Initialize GPIO pins for ADC (they must be set to input, no pull)
    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);

    // Initialize pad states
    for (int i = 0; i < NUM_PADS; i++) {
        pads[i].is_touched = false;
        pads[i].debounce_cnt = 0;
        pads[i].velocity = 0;
        pads[i].raw_value = 0;
    }
}

//--------------------------------------------------------------------+
// Pad scanning and debouncing
//--------------------------------------------------------------------+

void scan_pads(void) {
    for (int i = 0; i < NUM_PADS; i++) {
        // Select ADC input
        if (i < 3) {
            // ADC channels 0, 1, 2 on GPIO 26, 27, 28
            adc_select_input(i);
        } else {
            // Channel 4 = internal temperature sensor (not a touch pad)
            // Skip or use as a bonus sensor
            adc_select_input(4);
        }

        // Read ADC (12-bit value, 0-4095)
        uint16_t raw = adc_read();
        pads[i].raw_value = raw;

        // Determine if touched (voltage dropped below threshold)
        bool currently_touched = (raw < TOUCH_THRESHOLD);

        // Calculate velocity from ADC reading
        // Lower voltage = more touch = higher velocity
        if (currently_touched) {
            uint16_t range = TOUCH_THRESHOLD;
            uint16_t inverted = TOUCH_THRESHOLD - raw;
            uint8_t vel = (uint8_t)((uint32_t)inverted * MAX_VELOCITY / range);
            if (vel < MIN_VELOCITY) vel = MIN_VELOCITY;
            if (vel > MAX_VELOCITY) vel = MAX_VELOCITY;
            pads[i].velocity = vel;
        }

        // Debounce logic
        if (currently_touched) {
            if (pads[i].debounce_cnt < DEBOUNCE_COUNT) {
                pads[i].debounce_cnt++;
            }
            if (pads[i].debounce_cnt >= DEBOUNCE_COUNT && !pads[i].is_touched) {
                // Pad just touched -> send Note On
                pads[i].is_touched = true;
                send_midi_note_on(0, midi_notes[i], pads[i].velocity);
                // Also send CC with velocity value for expressiveness
                send_midi_cc(0, midi_notes[i] + 32, pads[i].velocity);
            } else if (pads[i].is_touched) {
                // Pad is still touched -> send CC with current pressure
                send_midi_cc(0, midi_notes[i] + 32, pads[i].velocity);
            }
        } else {
            if (pads[i].debounce_cnt > 0) {
                pads[i].debounce_cnt--;
            }
            if (pads[i].debounce_cnt == 0 && pads[i].is_touched) {
                // Pad just released -> send Note Off
                pads[i].is_touched = false;
                send_midi_note_off(0, midi_notes[i]);
            }
        }
    }
}

//--------------------------------------------------------------------+
// MIDI helpers
//--------------------------------------------------------------------+

void send_midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t const cable_num = 0;
    uint8_t note_on[3] = { 0x90 | channel, note, velocity };
    tud_midi_stream_write(cable_num, note_on, 3);
}

void send_midi_note_off(uint8_t channel, uint8_t note) {
    uint8_t const cable_num = 0;
    uint8_t note_off[3] = { 0x80 | channel, note, 0 };
    tud_midi_stream_write(cable_num, note_off, 3);
}

void send_midi_cc(uint8_t channel, uint8_t cc, uint8_t value) {
    uint8_t const cable_num = 0;
    uint8_t cc_msg[3] = { 0xB0 | channel, cc, value };
    tud_midi_stream_write(cable_num, cc_msg, 3);
}

//--------------------------------------------------------------------+
// MAIN
//--------------------------------------------------------------------+

int main(void) {
    board_init();

    // Initialize USB stack
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    // Initialize ADC for touch pads
    adc_init_pads();

    // Optional: init stdio for debugging (uses USB CDC if enabled)
    // stdio_init_all();

    while (1) {
        tud_task();       // TinyUSB device task
        led_blinking_task();
        midi_task();
    }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

void tud_mount_cb(void) {
    blink_interval_ms = BLINK_MOUNTED;
}

void tud_umount_cb(void) {
    blink_interval_ms = BLINK_NOT_MOUNTED;
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    blink_interval_ms = BLINK_SUSPENDED;
}

void tud_resume_cb(void) {
    blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// MIDI Task
//--------------------------------------------------------------------+

void midi_task(void) {
    static uint32_t start_ms = 0;

    // Drain any incoming MIDI (we don't process it, but must read it)
    while (tud_midi_available()) {
        uint8_t packet[4];
        tud_midi_packet_read(packet);
    }

    // Scan pads at configured interval
    if (board_millis() - start_ms < SCAN_INTERVAL_MS) return;
    start_ms += SCAN_INTERVAL_MS;

    scan_pads();
}

//--------------------------------------------------------------------+
// LED Blinking Task
//--------------------------------------------------------------------+

void led_blinking_task(void) {
    static uint32_t start_ms = 0;
    static bool led_state = false;

    if (!blink_interval_ms) return;
    if (board_millis() - start_ms < blink_interval_ms) return;
    start_ms += blink_interval_ms;

    board_led_write(led_state);
    led_state = 1 - led_state;
}
