/*
 * ADC + OLED SSD1306 - Raspberry Pi Pico RP2040
 *
 * Lee el nivel de sonido desde un módulo KY-037 conectado al ADC (GPIO 27)
 * y muestra el valor en tiempo real en una pantalla OLED SSD1306 128x64
 * conectada por I2C. También muestra el estado digital del KY-037.
 * Envía datos de debug por USB CDC (Virtual Serial).
 *
 * Basado en el ejemplo oficial: pico-examples/i2c/ssd1306_i2c
 *
 * Hardware:
 *   - Módulo KY-037 (sensor de sonido):
 *     - AO (salida analógica) → GPIO 27 (ADC1)
 *     - DO (salida digital)   → GPIO 26 (entrada digital)
 *   - OLED SSD1306:
 *     - SDA → GPIO 16 (I2C0)
 *     - SCL → GPIO 17 (I2C0)
 *   - Alimentación: 3.3V y GND del Pico
 *   - USB: Conexión USB para datos serial (CDC)
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "ssd1306_font.h"

//====================================================================+
// CONFIGURATION
//====================================================================+

// ADC Configuration (KY-037 analog output)
#define ADC_PIN            27      // GPIO 27 (ADC1) - KY-037 AO
#define ADC_CHANNEL        1       // ADC channel 1
#define ADC_VREF           3.3f    // Reference voltage
#define ADC_RESOLUTION     4095.0f // 12-bit ADC

// Digital input (KY-037 digital output)
#define KY037_DO_PIN       26      // GPIO 26 - KY-037 DO

// I2C Configuration for SSD1306
#define I2C_PORT           i2c0
#define I2C_SDA_PIN        16      // GPIO 16 (I2C0 SDA)
#define I2C_SCL_PIN        17      // GPIO 17 (I2C0 SCL)
#define I2C_BAUDRATE       400000  // 400 kHz

// OLED Configuration
#define SSD1306_HEIGHT     64
#define SSD1306_WIDTH      128
#define SSD1306_I2C_ADDR   0x3C

// Display refresh interval
#define DISPLAY_UPDATE_MS  300     // Update every 300ms (~3.3 FPS)

// Serial debug interval
#define SERIAL_UPDATE_MS   500     // Send debug data every 500ms (2 Hz)

// Peak detection threshold
#define PEAK_THRESHOLD     200     // Minimum ADC increase to count as peak
#define NOISE_FLOOR        100     // Ignore changes smaller than this

// ADC averaging
#define ADC_SAMPLES_AVG    16      // Number of ADC samples to average

// Firmware version
#define FIRMWARE_VERSION   "v1.1"

//====================================================================+
// SSD1306 DRIVER (from pico-examples)
//=====================================================================

#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)
#define SSD1306_SET_DISP_START_LINE _u(0x40)
#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)
#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)
#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_PAGE_HEIGHT         _u(8)
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    int buflen;
};

void calc_render_area_buflen(struct render_area *area) {
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

void SSD1306_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num) {
    for (int i = 0; i < num; i++)
        SSD1306_send_cmd(buf[i]);
}

void SSD1306_send_buf(uint8_t buf[], int buflen) {
    uint8_t *temp_buf = malloc(buflen + 1);
    temp_buf[0] = 0x40;
    memcpy(temp_buf + 1, buf, buflen);
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);
    free(temp_buf);
}

void SSD1306_init(void) {
    uint8_t cmds[] = {
        SSD1306_SET_DISP,               // set display off
        SSD1306_SET_MEM_MODE,           // set memory address mode 0 = horizontal
        0x00,                           // horizontal addressing mode
        SSD1306_SET_DISP_START_LINE,    // set display start line to 0
        SSD1306_SET_SEG_REMAP | 0x01,   // set segment re-map
        SSD1306_SET_MUX_RATIO,          // set multiplex ratio
        SSD1306_HEIGHT - 1,             // Display height - 1
        SSD1306_SET_COM_OUT_DIR | 0x08, // set COM output scan direction
        SSD1306_SET_DISP_OFFSET,        // set display offset
        0x00,                           // no offset
        SSD1306_SET_COM_PIN_CFG,        // set COM pins hardware configuration
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
        0x12,
#else
        0x02,
#endif
        SSD1306_SET_DISP_CLK_DIV,       // set display clock divide ratio
        0x80,                           // div ratio of 1, standard freq
        SSD1306_SET_PRECHARGE,          // set pre-charge period
        0xF1,                           // Vcc internally generated on our board
        SSD1306_SET_VCOM_DESEL,         // set VCOMH deselect level
        0x30,                           // 0.83xVcc
        SSD1306_SET_CONTRAST,           // set contrast control
        0xFF,
        SSD1306_SET_ENTIRE_ON,          // set entire display on to follow RAM content
        SSD1306_SET_NORM_DISP,           // set normal (not inverted) display
        SSD1306_SET_CHARGE_PUMP,        // set charge pump
        0x14,                           // Vcc internally generated on our board
        SSD1306_SET_SCROLL | 0x00,      // deactivate horizontal scrolling
        SSD1306_SET_DISP | 0x01,        // turn display on
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void SSD1306_scroll(bool on) {
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00, // dummy byte
        0x00, // start page 0
        0x00, // time interval
        SSD1306_NUM_PAGES - 1, // end page
        0x00, // dummy byte
        0xFF, // dummy byte
        SSD1306_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
    };
    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void render(uint8_t *buf, struct render_area *area) {
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page
    };
    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(buf, area->buflen);
}

static void SetPixel(uint8_t *buf, int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    const int BytesPerRow = SSD1306_WIDTH;
    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];
    if (on)
        byte |= 1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));
    buf[byte_idx] = byte;
}

static void DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (true) {
        SetPixel(buf, x0, y0, on);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static inline int GetFontIndex(uint8_t ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 1;
    else if (ch >= '0' && ch <= '9') return ch - '0' + 27;
    else return 0;
}

static void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8) return;
    y = y / 8;
    ch = toupper(ch);
    int idx = GetFontIndex(ch);
    int fb_idx = y * 128 + x;
    for (int i = 0; i < 8; i++) {
        buf[fb_idx++] = font[idx * 8 + i];
    }
}

static void WriteString(uint8_t *buf, int16_t x, int16_t y, char *str) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8) return;
    while (*str) {
        WriteChar(buf, x, y, *str++);
        x += 8;
    }
}

//====================================================================+
// USB CDC HELPERS
//====================================================================+

static void cdc_init(void) {
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);
}

static void cdc_send_string(const char *str) {
    if (!tud_cdc_connected()) return;
    tud_cdc_write(str, strlen(str));
    tud_cdc_write_flush();
}

//====================================================================+
// GLOBAL VARIABLES
//=====================================================================

static uint16_t current_adc_value = 0;
static float current_voltage = 0.0f;
static bool ky037_digital_state = false;
static bool oled_ok = false;

// Peak detection state
static uint16_t last_adc_value = 0;
static bool last_do_state = false;
static uint16_t noise_floor = 0;

// ECG buffer for display
#define ECG_BUFFER_SIZE   128
static uint16_t ecg_buffer[ECG_BUFFER_SIZE];
static uint8_t ecg_index = 0;

//====================================================================+
// ADC FUNCTIONS
//====================================================================+

static void adc_init_sensor(void) {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);
    gpio_init(KY037_DO_PIN);
    gpio_set_dir(KY037_DO_PIN, GPIO_IN);
}

static void adc_read_sensor(void) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < ADC_SAMPLES_AVG; i++) {
        adc_select_input(ADC_CHANNEL);
        sum += adc_read();
    }
    uint16_t raw = (uint16_t)(sum / ADC_SAMPLES_AVG);
    current_adc_value = raw;
    current_voltage = (raw * ADC_VREF) / ADC_RESOLUTION;
}

//====================================================================+
// MAIN
//====================================================================+

int main(void) {
    stdio_init_all();

    // Initialize USB CDC first
    cdc_init();
    sleep_ms(500);

    cdc_send_string("\r\n");
    cdc_send_string("========================================\r\n");
    cdc_send_string("  ADC + OLED + KY-037\r\n");
    cdc_send_string("  Raspberry Pi Pico RP2040\r\n");
    cdc_send_string("========================================\r\n");

    // Initialize ADC and digital input for KY-037
    adc_init_sensor();
    cdc_send_string("[ADC] KY-037 AO=GP27, DO=GP26 initialized\r\n");

    // Initialize I2C0 on GPIO 16/17 for OLED
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Scan for OLED
    bool found = false;
    for (uint8_t addr = 0x3C; addr <= 0x3D; addr++) {
        uint8_t buf[1] = {0x00};
        int ret = i2c_write_blocking(I2C_PORT, addr, buf, 1, false);
        if (ret >= 0) {
            char addr_buf[32];
            snprintf(addr_buf, sizeof(addr_buf), "[OLED] Found at 0x%02X\r\n", addr);
            cdc_send_string(addr_buf);
            found = true;
            break;
        }
    }

    if (!found) {
        cdc_send_string("[OLED] Device not found at 0x3C or 0x3D\r\n");
    }

    // Initialize OLED display
    SSD1306_init();
    oled_ok = found;

    // Initialize render area for entire frame
    struct render_area frame_area = {
        .start_col = 0,
        .end_col = SSD1306_WIDTH - 1,
        .start_page = 0,
        .end_page = SSD1306_NUM_PAGES - 1
    };
    calc_render_area_buflen(&frame_area);

    // Zero the entire display buffer
    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);

    // Show firmware version on OLED
    if (oled_ok) {
        WriteString(buf, 0, 0, "ADC+OLED+KY037");
        WriteString(buf, 0, 8, "Firmware:");
        WriteString(buf, 40, 16, FIRMWARE_VERSION);
        WriteString(buf, 0, 32, "Iniciando...");
        render(buf, &frame_area);
        sleep_ms(1500);
    }

    cdc_send_string("[SYS] Ready. Starting main loop...\r\n");
    cdc_send_string("========================================\r\n");

    // Main loop
    uint32_t last_display_ms = 0;
    uint32_t last_serial_ms = 0;
    uint32_t loop_count = 0;
    bool first_sample = true;

    while (1) {
        tud_task();
        uint32_t now = board_millis();

        // Read the ADC sensor
        adc_read_sensor();
        bool current_do_state = gpio_get(KY037_DO_PIN);

        // Initialize noise floor on first sample
        if (first_sample) {
            last_adc_value = current_adc_value;
            last_do_state = current_do_state;
            noise_floor = current_adc_value;
            first_sample = false;
        }

        // Detect LOW -> HIGH edge on DO (GPIO 26)
        bool do_rising_edge = (!last_do_state && current_do_state);

        // Detect ADC peak: significant increase from last value
        uint16_t adc_delta = 0;
        if (current_adc_value > last_adc_value) {
            adc_delta = current_adc_value - last_adc_value;
        }
        bool adc_peak = (adc_delta > PEAK_THRESHOLD) && (adc_delta > NOISE_FLOOR);

        // Update noise floor slowly (running minimum-ish)
        if (current_adc_value < noise_floor) {
            noise_floor = current_adc_value;
        } else if (noise_floor < PEAK_THRESHOLD) {
            noise_floor += 1;
        }

        // Send immediate event on DO rising edge
        if (do_rising_edge) {
            char event_buf[128];
            snprintf(event_buf, sizeof(event_buf),
                "[EVENT] DO RISING EDGE | ADC=%4u | V=%0.2fV\r\n",
                current_adc_value, current_voltage);
            cdc_send_string(event_buf);
        }

        // Send immediate event on ADC peak
        if (adc_peak) {
            char event_buf[128];
            snprintf(event_buf, sizeof(event_buf),
                "[PEAK] ADC PEAK +%u | ADC=%4u | V=%0.2fV | DO=%s\r\n",
                adc_delta, current_adc_value, current_voltage,
                current_do_state ? "HIGH" : "LOW ");
            cdc_send_string(event_buf);
        }

        // Store current state for next iteration
        last_adc_value = current_adc_value;
        last_do_state = current_do_state;
        ky037_digital_state = current_do_state;

        // Update ECG buffer
        ecg_buffer[ecg_index] = current_adc_value;
        ecg_index = (ecg_index + 1) % ECG_BUFFER_SIZE;

        // Update display periodically
        if (now - last_display_ms >= DISPLAY_UPDATE_MS) {
            last_display_ms = now;
            loop_count++;

            memset(buf, 0, SSD1306_BUF_LEN);

            if (!oled_ok) {
                WriteString(buf, 0, 0, "OLED NOT FOUND");
                WriteString(buf, 0, 8, "Check wiring:");
                WriteString(buf, 0, 16, "SDA=GP16 SCL=GP17");
                WriteString(buf, 0, 24, "Addr: 0x3C/0x3D");
            } else {
                char line[32];

                // Title
                WriteString(buf, 0, 0, "KY-037 ECG");

                // ADC value
                snprintf(line, sizeof(line), "ADC:%4u", current_adc_value);
                WriteString(buf, 0, 8, line);

                // Voltage
                snprintf(line, sizeof(line), "V:%0.2fV", current_voltage);
                WriteString(buf, 0, 16, line);

                // Digital state
                snprintf(line, sizeof(line), "DO:%s", ky037_digital_state ? "HIGH" : "LOW ");
                WriteString(buf, 0, 24, line);

                // ECG line graph
                uint8_t ecg_y_base = 40;
                uint8_t ecg_height = 20;
                for (uint8_t x = 0; x < SSD1306_WIDTH; x++) {
                    uint8_t idx = (ecg_index + x) % ECG_BUFFER_SIZE;
                    uint16_t val = ecg_buffer[idx];
                    uint8_t y = ecg_y_base + ecg_height - ((val * ecg_height) / 4095);
                    if (y < ecg_y_base) y = ecg_y_base;
                    if (y >= ecg_y_base + ecg_height) y = ecg_y_base + ecg_height - 1;
                    SetPixel(buf, x, y, true);
                }

                // ECG baseline
                for (uint8_t x = 0; x < SSD1306_WIDTH; x++) {
                    SetPixel(buf, x, ecg_y_base + ecg_height - 1, true);
                }
            }

            render(buf, &frame_area);
        }

        // Send debug data over serial periodically
        if (now - last_serial_ms >= SERIAL_UPDATE_MS) {
            last_serial_ms = now;
            char serial_buf[128];
            snprintf(serial_buf, sizeof(serial_buf),
                "[DATA] ADC=%4u | V=%0.2fV | DO=%s | OLED=%s\r\n",
                current_adc_value,
                current_voltage,
                ky037_digital_state ? "HIGH" : "LOW ",
                oled_ok ? "OK" : "FAIL");
            cdc_send_string(serial_buf);
        }
    }

    return 0;
}

//====================================================================+
// USB DEVICE CALLBACKS
//====================================================================+

void tud_mount_cb(void) {
    cdc_send_string("[USB] Device mounted\r\n");
}

void tud_umount_cb(void) {
    cdc_send_string("[USB] Device unmounted\r\n");
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    cdc_send_string("[USB] Device suspended\r\n");
}

void tud_resume_cb(void) {
    cdc_send_string("[USB] Device resumed\r\n");
}
