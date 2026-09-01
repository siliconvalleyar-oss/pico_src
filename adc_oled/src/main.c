/*
 * ADC + OLED SSD1306 - Raspberry Pi Pico RP2040
 *
 * Osciloscopio digital con KY-037.
 * - Izquierda arriba: voltaje ADC promedio (3 decimales)
 * - Derecha arriba: indicador digital DO (cuadrado vacío/lleno)
 * - Abajo: trazo tipo osciloscopio, 512ms de ventana
 * - Trigger por caída de señal: se dispara cuando el ADC baja
 *   por debajo del nivel configurado
 * - USB CDC para configuración de trigger y escala
 *
 * Comandos serial:
 *   TRIG:1.700   -> setea nivel de trigger en voltios
 *   SCALE:50     -> setea escala/amplitud (1, 10, 30, 50, 100)
 *   GET          -> muestra configuración actual
 *
 * Hardware:
 *   - KY-037 AO -> GPIO 27 (ADC1)
 *   - KY-037 DO -> GPIO 26
 *   - OLED SDA  -> GPIO 16
 *   - OLED SCL  -> GPIO 17
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

// ADC Configuration
#define ADC_PIN            27      // GPIO 27 (ADC1) - KY-037 AO
#define ADC_CHANNEL        1       // ADC channel 1
#define ADC_VREF           3.3f    // Reference voltage
#define ADC_RESOLUTION     4095.0f // 12-bit ADC

// Digital input
#define KY037_DO_PIN       26      // GPIO 26 - KY-037 DO

// I2C Configuration
#define I2C_PORT           i2c0
#define I2C_SDA_PIN        16      // GPIO 16 (I2C0 SDA)
#define I2C_SCL_PIN        17      // GPIO 17 (I2C0 SCL)
#define I2C_BAUDRATE       400000  // 400 kHz

// OLED Configuration
#define SSD1306_HEIGHT     64
#define SSD1306_WIDTH      128
#define SSD1306_I2C_ADDR   0x3C

// Timebase: 128 samples at 4ms = 512ms window
#define SAMPLE_INTERVAL_MS 4       // ~250 Hz
#define DISPLAY_UPDATE_MS  200     // Refresh display every 200ms
#define SERIAL_UPDATE_MS   1000    // Serial debug every 1s

// Trigger configuration
#define TRIGGER_HOLD_MS    300     // Block retrigger for 300ms

// Oscilloscope buffer
#define SCOPE_BUFFER_SIZE  128
static uint16_t scope_buffer[SCOPE_BUFFER_SIZE];
static uint8_t scope_index = 0;
static bool scope_triggered = false;

//====================================================================+
// GLOBAL VARIABLES
//====================================================================+

static uint16_t current_adc_value = 0;
static float current_voltage = 0.0f;
static bool ky037_digital_state = false;
static bool oled_ok = false;

// Trigger configuration
static uint16_t trigger_voltage_mv = 1700;
static float trigger_scale = 10.0f;
static uint16_t trigger_adc_level = 0;
static bool monitor_enabled = false;
static uint8_t trigger_mode = 0; // 0 = active low (falling), 1 = active high (rising)

// Oscilloscope state
static uint16_t adc_sum = 0;
static uint16_t adc_samples = 0;
static uint16_t adc_average = 0;
static uint16_t last_adc_value = 0;
static bool last_do_state = false;
static uint16_t noise_floor = 0;
static uint32_t last_trigger_ms = 0;

//====================================================================+
// ADC FUNCTIONS
//====================================================================+

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

static void DrawCircle(uint8_t *buf, int x0, int y0, int radius, bool filled) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        if (filled) {
            DrawLine(buf, x0 - x, y0 + y, x0 + x, y0 + y, true);
            DrawLine(buf, x0 - y, y0 + x, x0 + y, y0 + x, true);
            DrawLine(buf, x0 - x, y0 - y, x0 + x, y0 - y, true);
            DrawLine(buf, x0 - y, y0 - x, x0 + y, y0 - x, true);
        } else {
            SetPixel(buf, x0 + x, y0 + y, true);
            SetPixel(buf, x0 + y, y0 + x, true);
            SetPixel(buf, x0 - y, y0 + x, true);
            SetPixel(buf, x0 - x, y0 + y, true);
            SetPixel(buf, x0 - x, y0 - y, true);
            SetPixel(buf, x0 - y, y0 - x, true);
            SetPixel(buf, x0 + y, y0 - x, true);
            SetPixel(buf, x0 + x, y0 - y, true);
        }

        y += 1;
        if (err <= 0) {
            err += 2 * y + 1;
            continue;
        }
        x -= 1;
        err += 2 * (y - x) + 1;
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
// SERIAL COMMAND PARSER
//====================================================================+

static void process_serial_command(const char *cmd) {
    if (strncmp(cmd, "TRIG:", 5) == 0) {
        float voltage = atof(cmd + 5);
        if (voltage >= 0.0f && voltage <= 3.3f) {
            trigger_voltage_mv = (uint16_t)(voltage * 1000.0f);
            trigger_adc_level = (uint16_t)((trigger_voltage_mv / 1000.0f) * ADC_RESOLUTION / ADC_VREF);
            char resp[64];
            snprintf(resp, sizeof(resp), "[CFG] Trigger set to %.3fV (%u ADC)\r\n",
                voltage, trigger_adc_level);
            cdc_send_string(resp);
        } else {
            cdc_send_string("[CFG] Error: voltage must be 0.000 - 3.300V\r\n");
        }
    } else if (strncmp(cmd, "TRIGMODE:", 9) == 0) {
        uint8_t mode = atoi(cmd + 9);
        if (mode <= 1) {
            trigger_mode = mode;
            char resp[64];
            snprintf(resp, sizeof(resp), "[CFG] Trigger mode set to %s\r\n",
                mode == 0 ? "ACTIVE LOW (falling)" : "ACTIVE HIGH (rising)");
            cdc_send_string(resp);
        } else {
            cdc_send_string("[CFG] Error: TRIGMODE must be 0 or 1\r\n");
        }
    } else if (strncmp(cmd, "SCALE:", 6) == 0) {
        float scale = atof(cmd + 6);
        if (scale >= 0.1f && scale <= 100.0f) {
            trigger_scale = scale;
            char resp[64];
            snprintf(resp, sizeof(resp), "[CFG] Scale set to %.1f\r\n", scale);
            cdc_send_string(resp);
        } else {
            cdc_send_string("[CFG] Error: scale must be 0.1 - 100.0\r\n");
        }
    } else if (strncmp(cmd, "MONITOR:", 8) == 0) {
        if (strcmp(cmd + 8, "ON") == 0) {
            monitor_enabled = true;
            cdc_send_string("[CFG] Monitor enabled\r\n");
        } else if (strcmp(cmd + 8, "OFF") == 0) {
            monitor_enabled = false;
            cdc_send_string("[CFG] Monitor disabled\r\n");
        } else {
            cdc_send_string("[CFG] Error: use MONITOR:ON or MONITOR:OFF\r\n");
        }
    } else if (strcmp(cmd, "VERSION") == 0) {
        cdc_send_string("[VERSION] adc_oled v1.2.0\r\n");
    } else if (strcmp(cmd, "GET") == 0) {
        char resp[128];
        snprintf(resp, sizeof(resp),
            "[CFG] Trigger=%.3fV (%u ADC) | Mode=%s | Scale=%.1f | Monitor=%s | NoiseFloor=%u\r\n",
            trigger_voltage_mv / 1000.0f, trigger_adc_level,
            trigger_mode == 0 ? "LOW" : "HIGH",
            trigger_scale, monitor_enabled ? "ON" : "OFF", noise_floor);
        cdc_send_string(resp);
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
// ADC FUNCTIONS
//====================================================================+

static void adc_init_sensor(void) {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);
    gpio_init(KY037_DO_PIN);
    gpio_set_dir(KY037_DO_PIN, GPIO_IN);

#if defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
#endif
}

static void adc_sample(void) {
    adc_select_input(ADC_CHANNEL);
    uint16_t raw = adc_read();
    current_adc_value = raw;
    current_voltage = (raw * ADC_VREF) / ADC_RESOLUTION;

    adc_sum += raw;
    adc_samples++;

    scope_buffer[scope_index] = raw;
    scope_index = (scope_index + 1) % SCOPE_BUFFER_SIZE;
}

//====================================================================+
// TRIGGER DETECTION (falling edge below level)
//=====================================================================

static bool detect_trigger(uint32_t now) {
    bool trigger_event = false;
    bool armed = (now - last_trigger_ms) > TRIGGER_HOLD_MS;

    if (trigger_mode == 0) {
        // Active LOW: trigger when signal falls below level
        bool below_level = current_adc_value < trigger_adc_level;
        bool was_above = last_adc_value >= trigger_adc_level;
        trigger_event = below_level && was_above && armed;
    } else {
        // Active HIGH: trigger when signal rises above level
        bool above_level = current_adc_value > trigger_adc_level;
        bool was_below = last_adc_value <= trigger_adc_level;
        trigger_event = above_level && was_below && armed;
    }

    if (trigger_event) {
        last_trigger_ms = now;
        return true;
    }

    return false;
}

//====================================================================+
// MAIN
//====================================================================+

int main(void) {
    stdio_init_all();

    cdc_init();
    sleep_ms(500);

    cdc_send_string("\r\n");
    cdc_send_string("========================================\r\n");
    cdc_send_string("  KY-037 Oscilloscope\r\n");
    cdc_send_string("  Raspberry Pi Pico RP2040\r\n");
    cdc_send_string("========================================\r\n");
    cdc_send_string("[CFG] Default trigger: 1.700V\r\n");
    cdc_send_string("[CFG] Commands: TRIG:1.700, SCALE:50, GET\r\n");

    adc_init_sensor();
    cdc_send_string("[ADC] KY-037 AO=GP27, DO=GP26 initialized\r\n");

    i2c_init(I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

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

    SSD1306_init();
    oled_ok = found;

    struct render_area frame_area = {
        .start_col = 0,
        .end_col = SSD1306_WIDTH - 1,
        .start_page = 0,
        .end_page = SSD1306_NUM_PAGES - 1
    };
    calc_render_area_buflen(&frame_area);

    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);

    if (oled_ok) {
        WriteString(buf, 0, 0, "KY-037 OSC");
        WriteString(buf, 0, 8, "Iniciando...");
        render(buf, &frame_area);
        sleep_ms(1000);
    }

    cdc_send_string("[SYS] Ready. Starting main loop...\r\n");
    cdc_send_string("========================================\r\n");

    uint32_t last_sample_ms = 0;
    uint32_t last_display_ms = 0;
    uint32_t last_serial_ms = 0;
    bool first_sample = true;

    while (1) {
        tud_task();
        uint32_t now = board_millis();

        // Check for serial commands
        check_serial_commands();

        // Sample ADC at fixed interval (~250 Hz)
        if (now - last_sample_ms >= SAMPLE_INTERVAL_MS) {
            last_sample_ms = now;

            adc_sample();
            bool current_do_state = gpio_get(KY037_DO_PIN);

            if (first_sample) {
                last_adc_value = current_adc_value;
                last_do_state = current_do_state;
                noise_floor = current_adc_value;
                // Compute initial trigger ADC level
                trigger_adc_level = (uint16_t)((trigger_voltage_mv / 1000.0f) * ADC_RESOLUTION / ADC_VREF);
                first_sample = false;
            }

            // Trigger detection: falling edge below trigger level
            bool trigger = detect_trigger(now);
            if (trigger) {
                memset(scope_buffer, 0, sizeof(scope_buffer));
                scope_index = 0;
                scope_triggered = true;
            }

            last_adc_value = current_adc_value;
            last_do_state = current_do_state;
            ky037_digital_state = current_do_state;

#if defined(PICO_DEFAULT_LED_PIN)
            gpio_put(PICO_DEFAULT_LED_PIN, ky037_digital_state ? 1 : 0);
#endif

            // Update noise floor
            if (current_adc_value < noise_floor) {
                noise_floor = current_adc_value;
            } else if (noise_floor < trigger_adc_level) {
                noise_floor++;
            }
        }

        // Update display every 200ms
        if (now - last_display_ms >= DISPLAY_UPDATE_MS) {
            last_display_ms = now;
            memset(buf, 0, SSD1306_BUF_LEN);

            if (!oled_ok) {
                WriteString(buf, 0, 0, "OLED NOT FOUND");
                WriteString(buf, 0, 8, "Check wiring:");
                WriteString(buf, 0, 16, "SDA=GP16 SCL=GP17");
                WriteString(buf, 0, 24, "Addr: 0x3C/0x3D");
            } else {
                char line[32];

                // Title / ADC voltage (top left)
                if (adc_samples > 0) {
                    adc_average = (uint16_t)(adc_sum / adc_samples);
                }
                snprintf(line, sizeof(line), "V:%0.3fV", (adc_average * ADC_VREF) / ADC_RESOLUTION);
                WriteString(buf, 0, 0, line);

                // Trigger level indicator (top middle)
                snprintf(line, sizeof(line), "T:%.3fV", trigger_voltage_mv / 1000.0f);
                WriteString(buf, 42, 0, line);

                // DO indicator (top right): empty square for LOW, filled for HIGH
                if (ky037_digital_state) {
                    DrawCircle(buf, 120, 4, 4, true);
                } else {
                    DrawCircle(buf, 120, 4, 4, false);
                }

                // Oscilloscope area: y=16..63 (48 pixels high), x=0..127 (128 pixels wide)
                uint8_t trace_y = 16;
                uint8_t trace_h = 48;

                // Draw waveform as connected lines
                if (scope_triggered && scope_index < SCOPE_BUFFER_SIZE) {
                    // Post-trigger: show filled samples from left
                    for (uint8_t i = 1; i < scope_index && i < SSD1306_WIDTH; i++) {
                        uint16_t val0 = scope_buffer[i - 1];
                        uint16_t val1 = scope_buffer[i];
                        uint8_t y0 = trace_y + trace_h - ((val0 * trace_h) / 4095);
                        uint8_t y1 = trace_y + trace_h - ((val1 * trace_h) / 4095);
                        if (y0 < trace_y) y0 = trace_y;
                        if (y0 >= trace_y + trace_h) y0 = trace_y + trace_h - 1;
                        if (y1 < trace_y) y1 = trace_y;
                        if (y1 >= trace_y + trace_h) y1 = trace_y + trace_h - 1;
                        DrawLine(buf, i - 1, y0, i, y1, true);
                    }
                } else {
                    // Rolling buffer: show last 128 samples across 128 pixels
                    for (uint8_t x = 1; x < SSD1306_WIDTH; x++) {
                        uint8_t idx0 = (scope_index + x - 1) % SCOPE_BUFFER_SIZE;
                        uint8_t idx1 = (scope_index + x) % SCOPE_BUFFER_SIZE;
                        uint16_t val0 = scope_buffer[idx0];
                        uint16_t val1 = scope_buffer[idx1];
                        uint8_t y0 = trace_y + trace_h - ((val0 * trace_h) / 4095);
                        uint8_t y1 = trace_y + trace_h - ((val1 * trace_h) / 4095);
                        if (y0 < trace_y) y0 = trace_y;
                        if (y0 >= trace_y + trace_h) y0 = trace_y + trace_h - 1;
                        if (y1 < trace_y) y1 = trace_y;
                        if (y1 >= trace_y + trace_h) y1 = trace_y + trace_h - 1;
                        DrawLine(buf, x - 1, y0, x, y1, true);
                    }
                }

                // Trigger level line (dashed)
                uint8_t trig_y = trace_y + trace_h - ((trigger_adc_level * trace_h) / 4095);
                if (trig_y >= trace_y && trig_y < trace_y + trace_h) {
                    for (uint8_t x = 0; x < SSD1306_WIDTH; x += 4) {
                        SetPixel(buf, x, trig_y, true);
                    }
                }
            }

            render(buf, &frame_area);

            // Reset average accumulator
            adc_sum = 0;
            adc_samples = 0;
        }

        // Serial debug every 1s
        if (now - last_serial_ms >= SERIAL_UPDATE_MS) {
            last_serial_ms = now;
            if (monitor_enabled) {
                char serial_buf[128];
                snprintf(serial_buf, sizeof(serial_buf),
                    "[DATA] ADC=%4u | V=%0.3fV | TRIG=%u | DO=%s\r\n",
                    current_adc_value,
                    current_voltage,
                    trigger_adc_level,
                    ky037_digital_state ? "HIGH" : "LOW ");
                cdc_send_string(serial_buf);
            } else {
                cdc_send_string("[CFG] adc_oled v1.2.0\r\n");
            }
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
