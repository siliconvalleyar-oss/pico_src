/*
 * MIDI Touch Pads - Raspberry Pi Pico USB MIDI Controller
 *
 * Reads analog touch pads on ADC pins (GPIO 26, 27, 28) and converts
 * touch/no-touch into MIDI Note On/Off messages sent over USB.
 *
 * Features:
 *   - Multiple musical scales (Major, Minor, Pentatonic, Blues, etc.)
 *   - Octave shifting with button or MIDI
 *   - Velocity sensitivity from ADC pressure
 *   - Control Change messages for continuous control
 *
 * Hardware:
 *   - Touch pad on GPIO 26 (ADC0) -> Pad 1
 *   - Touch pad on GPIO 27 (ADC1) -> Pad 2
 *   - Touch pad on GPIO 28 (ADC2) -> Pad 3
 *   - Optional: internal temperature sensor as pad 4
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
#include "hardware/gpio.h"

//====================================================================+
// CONFIGURATION
//====================================================================+

// Number of touch pad channels
#define NUM_PADS            3   // Using 3 external pads (GPIO 26-28)

// ADC pins for touch pads
static const uint8_t adc_pins[NUM_PADS] = { 26, 27, 28 };

// ADC channel mapping
static const uint8_t adc_channels[NUM_PADS] = { 0, 1, 2 };

// Button to change scale (GPIO 15 - optional)
#define SCALE_BUTTON_PIN    15
#define SCALE_BUTTON_PULL   true

// ADC threshold - AJUSTADO PARA MAYOR SENSIBILIDAD
// ADC es 12-bit (0-4095). Sin toque: ~4095. Con toque: baja.
#define TOUCH_THRESHOLD     3500    // Detecta toques más ligeros
#define MIN_VELOCITY        20      // Velocity mínima más baja
#define MAX_VELOCITY        127
#define DEBOUNCE_COUNT      2       // Respuesta más rápida
#define SCAN_INTERVAL_MS    5       // Escaneo más rápido

//====================================================================+
// MUSICAL SCALES
//====================================================================+

// Scale definitions (intervals from root note)
// Each scale has 7 notes (one per degree of the scale)
// We use the first 3 notes for our 3 pads

typedef struct {
    const char *name;
    uint8_t intervals[7];   // Semitone intervals from root
    uint8_t num_notes;      // Number of notes in scale
} scale_t;

// Major scale: W-W-H-W-W-W-H (2-2-1-2-2-2-1)
static const scale_t SCALE_MAJOR = {
    .name = "Major",
    .intervals = { 0, 2, 4, 5, 7, 9, 11 },
    .num_notes = 7
};

// Natural Minor scale: W-H-W-W-H-W-W (2-1-2-2-1-2-2)
static const scale_t SCALE_MINOR = {
    .name = "Minor",
    .intervals = { 0, 2, 3, 5, 7, 8, 10 },
    .num_notes = 7
};

// Harmonic Minor: W-H-W-W-H-WH-H (2-1-2-2-1-3-1)
static const scale_t SCALE_HARMONIC_MINOR = {
    .name = "Harmonic Minor",
    .intervals = { 0, 2, 3, 5, 7, 8, 11 },
    .num_notes = 7
};

// Pentatonic Major: 5 notes (no 4th or 7th)
static const scale_t SCALE_PENTATONIC_MAJOR = {
    .name = "Pentatonic Major",
    .intervals = { 0, 2, 4, 7, 9 },
    .num_notes = 5
};

// Pentatonic Minor: 5 notes
static const scale_t SCALE_PENTATONIC_MINOR = {
    .name = "Pentatonic Minor",
    .intervals = { 0, 3, 5, 7, 10 },
    .num_notes = 5
};

// Blues scale: Minor pentatonic + b5
static const scale_t SCALE_BLUES = {
    .name = "Blues",
    .intervals = { 0, 3, 5, 6, 7, 10 },
    .num_notes = 6
};

// Dorian mode: like minor but with raised 6th
static const scale_t SCALE_DORIAN = {
    .name = "Dorian",
    .intervals = { 0, 2, 3, 5, 7, 9, 10 },
    .num_notes = 7
};

// Mixolydian mode: like major but with lowered 7th
static const scale_t SCALE_MIXOLYDIAN = {
    .name = "Mixolydian",
    .intervals = { 0, 2, 4, 5, 7, 9, 10 },
    .num_notes = 7
};

// Chromatic: all 12 semitones
static const scale_t SCALE_CHROMATIC = {
    .name = "Chromatic",
    .intervals = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
    .num_notes = 12
};

// Whole tone: 6 notes
static const scale_t SCALE_WHOLE_TONE = {
    .name = "Whole Tone",
    .intervals = { 0, 2, 4, 6, 8, 10 },
    .num_notes = 6
};

// Diminished: alternating whole/half steps
static const scale_t SCALE_DIMINISHED = {
    .name = "Diminished",
    .intervals = { 0, 2, 3, 5, 6, 8, 9, 11 },
    .num_notes = 8
};

// Japanese (In-Sen): traditional Japanese scale
static const scale_t SCALE_JAPANESE = {
    .name = "Japanese",
    .intervals = { 0, 1, 5, 7, 8 },
    .num_notes = 5
};

// Arabic (Hijaz): Middle Eastern sound
static const scale_t SCALE_ARABIC = {
    .name = "Arabic",
    .intervals = { 0, 1, 4, 5, 7, 8, 11 },
    .num_notes = 7
};

// Indian (Bhairav): Indian classical
static const scale_t SCALE_INDIAN = {
    .name = "Indian",
    .intervals = { 0, 1, 4, 5, 7, 8, 11 },
    .num_notes = 7
};

// All scales array
#define NUM_SCALES  14
static const scale_t *scales[NUM_SCALES] = {
    &SCALE_MAJOR,
    &SCALE_MINOR,
    &SCALE_HARMONIC_MINOR,
    &SCALE_PENTATONIC_MAJOR,
    &SCALE_PENTATONIC_MINOR,
    &SCALE_BLUES,
    &SCALE_DORIAN,
    &SCALE_MIXOLYDIAN,
    &SCALE_CHROMATIC,
    &SCALE_WHOLE_TONE,
    &SCALE_DIMINISHED,
    &SCALE_JAPANESE,
    &SCALE_ARABIC,
    &SCALE_INDIAN
};

//====================================================================+
// STATE VARIABLES
//====================================================================+

// Current scale index
static uint8_t current_scale = 0;

// Current root note (MIDI note number)
// C4 = 60, D4 = 62, E4 = 64, etc.
static uint8_t root_note = 60;  // C4

// Octave offset (0 = middle octave, -1 = lower, +1 = higher)
static int8_t octave_offset = 0;

// Current notes for pads (calculated from scale)
static uint8_t pad_notes[NUM_PADS];

// Button state for scale change
static bool last_button_state = false;

//====================================================================+
// BLINK PATTERN
//====================================================================+

enum {
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

//====================================================================+
// TOUCH PAD STATE
//====================================================================+

typedef struct {
    bool     is_touched;
    uint8_t  debounce_cnt;
    uint8_t  velocity;
    uint16_t raw_value;
} pad_state_t;

static pad_state_t pads[NUM_PADS];

//====================================================================+
// PROTOTYPES
//====================================================================+

void led_blinking_task(void);
void midi_task(void);
void adc_init_pads(void);
void scan_pads(void);
void update_pad_notes(void);
void check_scale_button(void);
void send_midi_note_on(uint8_t channel, uint8_t note, uint8_t velocity);
void send_midi_note_off(uint8_t channel, uint8_t note);
void send_midi_cc(uint8_t channel, uint8_t cc, uint8_t value);

//====================================================================+
// SCALE FUNCTIONS
//====================================================================+

// Calculate pad notes from current scale
void update_pad_notes(void) {
    const scale_t *scale = scales[current_scale];
    
    for (int i = 0; i < NUM_PADS; i++) {
        if (i < scale->num_notes) {
            // Use notes from the scale
            pad_notes[i] = root_note + scale->intervals[i] + (octave_offset * 12);
        } else {
            // If we have more pads than scale notes, wrap around
            uint8_t idx = i % scale->num_notes;
            uint8_t octave_up = i / scale->num_notes;
            pad_notes[i] = root_note + scale->intervals[idx] + 
                           (octave_offset * 12) + (octave_up * 12);
        }
        
        // Ensure note is in valid MIDI range (0-127)
        if (pad_notes[i] > 127) pad_notes[i] = 127;
    }
}

// Check button to change scale
void check_scale_button(void) {
    bool button_pressed = !gpio_get(SCALE_BUTTON_PIN);  // Active low with pull-up
    
    // Detect button press (falling edge)
    if (button_pressed && !last_button_state) {
        // Change to next scale
        current_scale = (current_scale + 1) % NUM_SCALES;
        update_pad_notes();
        
        // Flash LED to indicate scale change
        for (int i = 0; i <= current_scale; i++) {
            board_led_write(true);
            sleep_ms(100);
            board_led_write(false);
            sleep_ms(100);
        }
    }
    
    last_button_state = button_pressed;
}

//====================================================================+
// ADC INITIALIZATION
//====================================================================+

void adc_init_pads(void) {
    adc_init();

    // Initialize GPIO pins for ADC
    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);
    
    // Initialize scale button
    gpio_init(SCALE_BUTTON_PIN);
    gpio_set_dir(SCALE_BUTTON_PIN, GPIO_IN);
    if (SCALE_BUTTON_PULL) {
        gpio_pull_up(SCALE_BUTTON_PIN);
    }

    // Initialize pad states
    for (int i = 0; i < NUM_PADS; i++) {
        pads[i].is_touched = false;
        pads[i].debounce_cnt = 0;
        pads[i].velocity = 0;
        pads[i].raw_value = 0;
    }
    
    // Calculate initial pad notes
    update_pad_notes();
}

//====================================================================+
// PAD SCANNING - ALTA SENSIBILIDAD
//====================================================================+

void scan_pads(void) {
    for (int i = 0; i < NUM_PADS; i++) {
        // Select ADC input
        adc_select_input(adc_channels[i]);

        // Read ADC (12-bit value, 0-4095)
        // Sin toque: ~4095 (3.3V)
        // Con toque: baja hacia 0
        uint16_t raw = adc_read();
        pads[i].raw_value = raw;

        // Determine if touched - más sensible
        // Si el valor baja del umbral, hay toque
        bool currently_touched = (raw < TOUCH_THRESHOLD);

        // Calculate velocity - mejor escala
        if (currently_touched) {
            // Calcular velocity basado en cuánto bajó
            // raw: 0 (toque fuerte) a TOUCH_THRESHOLD (toque ligero)
            uint32_t drop = TOUCH_THRESHOLD - raw;
            uint32_t vel = (drop * MAX_VELOCITY) / TOUCH_THRESHOLD;
            
            if (vel < MIN_VELOCITY) vel = MIN_VELOCITY;
            if (vel > MAX_VELOCITY) vel = MAX_VELOCITY;
            pads[i].velocity = (uint8_t)vel;
        }

        // Debounce logic - más rápido
        if (currently_touched) {
            if (pads[i].debounce_cnt < DEBOUNCE_COUNT) {
                pads[i].debounce_cnt++;
            }
            if (pads[i].debounce_cnt >= DEBOUNCE_COUNT && !pads[i].is_touched) {
                // Pad just touched -> send Note On
                pads[i].is_touched = true;
                send_midi_note_on(0, pad_notes[i], pads[i].velocity);
                // Send CC with velocity
                send_midi_cc(0, pad_notes[i] + 32, pads[i].velocity);
            } else if (pads[i].is_touched) {
                // Pad is still touched -> send CC with current pressure
                send_midi_cc(0, pad_notes[i] + 32, pads[i].velocity);
            }
        } else {
            if (pads[i].debounce_cnt > 0) {
                pads[i].debounce_cnt--;
            }
            if (pads[i].debounce_cnt == 0 && pads[i].is_touched) {
                // Pad just released -> send Note Off
                pads[i].is_touched = false;
                send_midi_note_off(0, pad_notes[i]);
            }
        }
    }
}

//====================================================================+
// MIDI HELPERS
//====================================================================+

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

//====================================================================+
// MAIN
//====================================================================+

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

    while (1) {
        tud_task();
        led_blinking_task();
        midi_task();
        check_scale_button();
    }
}

//====================================================================+
// DEVICE CALLBACKS
//====================================================================+

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

//====================================================================+
// MIDI TASK
//====================================================================+

void midi_task(void) {
    static uint32_t start_ms = 0;

    // Drain incoming MIDI
    while (tud_midi_available()) {
        uint8_t packet[4];
        tud_midi_packet_read(packet);
        
        // Parse incoming MIDI for scale/root changes
        if (packet[0] == 0x90 && packet[2] > 0) {
            // Note On received - could use to change root note
            // packet[1] = note number
        }
        else if (packet[0] == 0xB0) {
            // CC received
            // CC 14 = change scale
            // CC 15 = change octave
            if (packet[1] == 14) {
                current_scale = packet[2] % NUM_SCALES;
                update_pad_notes();
            }
            else if (packet[1] == 15) {
                octave_offset = (int8_t)(packet[2] / 21) - 3;  // Map 0-127 to -3 to +3
                update_pad_notes();
            }
        }
    }

    // Scan pads
    if (board_millis() - start_ms < SCAN_INTERVAL_MS) return;
    start_ms += SCAN_INTERVAL_MS;

    scan_pads();
}

//====================================================================+
// LED BLINKING TASK
//====================================================================+

void led_blinking_task(void) {
    static uint32_t start_ms = 0;
    static bool led_state = false;

    if (!blink_interval_ms) return;
    if (board_millis() - start_ms < blink_interval_ms) return;
    start_ms += blink_interval_ms;

    board_led_write(led_state);
    led_state = 1 - led_state;
}
