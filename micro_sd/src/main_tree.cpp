/*
 * main_tree.cpp - Pico + microSD + OLED: vista "tree ." + USB MSC
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Igual que main.cpp pero el listado del OLED es un arbol recursivo:
 *
 *   MICROSD        SDHC N=12
 *   --------------------------------
 *   .
 *   +-CARPETAFOTOS/
 *   | '-IMG_001.JPG      2M
 *   +-MISDOC/
 *   '-README.TXT       512
 *
 * El arbol completo (aplanado, hasta TREE_MAX_NODES lineas) se desliza hacia
 * abajo cíclicamente: al llegar a la última línea vuelve a la primera.
 * USB MSC sigue igual: la PC ve el FAT de la tarjeta como pendrive.
 *
 * Compilacion: se elige en src/CMakeLists.txt (main_tree.cpp <-> main.cpp).
 */

#include <cstdio>
#include <cstring>
#include <cctype>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

extern "C" {
#include "ssd1306.h"
#include "sd_spi.h"
#include "tree_view.h"
#include "usb_msc.h"
}

#include "config.h"

// Filas del OLED para el arbol (debajo de cabecera y separador)
#define TREE_ROWS 6

static ssd1306_t oled;

//--------------------------------------------------------------------+
// LED de estado
//--------------------------------------------------------------------+
static void led_set(bool on) {
#ifdef CYW43_WL_GPIO_LED_PIN
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
#elif defined(PICO_DEFAULT_LED_PIN)
    gpio_put(PICO_DEFAULT_LED_PIN, on);
#else
    gpio_put(LED_PIN_FALLBACK, on);
#endif
}

static void led_init(void) {
#ifdef CYW43_WL_GPIO_LED_PIN
    if (cyw43_arch_init() != 0) {
        printf("aviso: cyw43_arch_init fallo; sin LED\r\n");
    }
#elif defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#else
    gpio_init(LED_PIN_FALLBACK);
    gpio_set_dir(LED_PIN_FALLBACK, GPIO_OUT);
#endif
}

//--------------------------------------------------------------------+
// Render del arbol: ventana deslizante ciclica sobre las lineas aplanadas
//--------------------------------------------------------------------+
static uint16_t s_top = 0;             // primera linea visible
static uint32_t s_last_step_ms = 0;

static void tree_render(void) {
    const tree_t *t = tree_get();

    ssd1306_clear(&oled);

    // Cabecera: label + tipo de tarjeta + cantidad de lineas
    char line[17];
    snprintf(line, sizeof(line), "%s", tree_volume_label());
    ssd1306_draw_string(&oled, 0, 0, line);
    snprintf(line, sizeof(line), "%s", sd_spi_type_name());
    ssd1306_draw_string(&oled, 56, 0, line);
    snprintf(line, sizeof(line), "N=%u", t->count);
    ssd1306_draw_string(&oled, 88, 0, line);

    ssd1306_draw_line(&oled, 0, 8, 127, 8, true);

    if (!t->valid || t->count == 0) {
        ssd1306_draw_string(&oled, 0, 20, "VACIO");
        ssd1306_update(&oled);
        return;
    }

    // Ventana de TREE_ROWS lineas, ciclica sobre el arbol aplanado
    for (int row = 0; row < TREE_ROWS; row++) {
        uint16_t idx = (uint16_t) ((s_top + row) % t->count);
        const tree_node_t *nd = &t->nodes[idx];

        int y = 10 + row * 8;

        if (nd->is_dir || nd->size == 0) {
            ssd1306_draw_string(&oled, 0, y, nd->text);
        } else {
            // Archivo: texto (rama+nombre) a la izquierda, tamano a la derecha
            char size[8];
            if      (nd->size >= 1024u * 1024u)
                snprintf(size, sizeof(size), "%luM", (unsigned long) (nd->size / (1024u * 1024u)));
            else if (nd->size >= 1024u)
                snprintf(size, sizeof(size), "%luK", (unsigned long) (nd->size / 1024u));
            else
                snprintf(size, sizeof(size), "%lu",  (unsigned long) nd->size);

            // El texto con rama puede ocupar toda la fila: recortar a 13
            char trunc[TREE_NAME_MAX + 8];
            snprintf(trunc, sizeof(trunc), "%-13.13s", nd->text);

            char full[24];
            snprintf(full, sizeof(full), "%s%4s", trunc, size);
            ssd1306_draw_string(&oled, 0, y, full);
        }
    }

    ssd1306_update(&oled);
}

static void tree_tick(uint32_t now_ms) {
    if (s_last_step_ms == 0) s_last_step_ms = now_ms;

    if (now_ms - s_last_step_ms >= BANNER_SCROLL_MS) {
        s_last_step_ms = now_ms;
        const tree_t *t = tree_get();
        if (t->count > 0) {
            s_top = (uint16_t) ((s_top + 1) % t->count);
        }
        tree_render();
    }
}

static void tree_reset(void) {
    s_top = 0;
    s_last_step_ms = 0;
    tree_render();
}

//--------------------------------------------------------------------+
// MAIN
//--------------------------------------------------------------------+
int main() {
    stdio_init_all();
    led_init();
    led_set(true);

    printf("\r\n=== PICO + microSD TREE + USB MSC ===\r\n");

    ssd1306_init(&oled, OLED_I2C_INST, OLED_SDA_PIN, OLED_SCL_PIN);
    ssd1306_clear(&oled);
    ssd1306_draw_string(&oled, 0, 0, "MICRO-SD");
    ssd1306_draw_line(&oled, 0, 16, 127, 16, true);
    ssd1306_draw_string(&oled, 0, 24, "TREE SCAN...");
    ssd1306_update(&oled);

    bool sd_ok = sd_spi_init();
    bool fs_ok = false;

    if (sd_ok) {
        printf("SD: %s, %u sectores\r\n", sd_spi_type_name(),
               (unsigned) sd_sector_count_cached());
        fs_ok = tree_scan();
    }

    if (!fs_ok) {
        printf("SD/FAT: no detectada (%s)\r\n", sd_ok ? "sin FAT" : "sin tarjeta");
        ssd1306_clear(&oled);
        ssd1306_draw_string(&oled, 0, 0, "MICRO-SD");
        ssd1306_draw_line(&oled, 0, 16, 127, 16, true);
        ssd1306_draw_string(&oled, 0, 28, "NO SD");
        ssd1306_draw_string(&oled, 0, 44, "INSERTAR");
        ssd1306_update(&oled);
    } else {
        printf("TREE: %u lineas%s\r\n", tree_get()->count,
               tree_get()->truncated ? " (truncado)" : "");
        tree_reset();
    }

    usb_msc_init();
    printf("USB MSC: listo\r\n");

    uint32_t last_rescan = to_ms_since_boot(get_absolute_time());
    uint32_t last_blink  = last_rescan;

    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        usb_msc_task();

        // Sin tarjeta: reintentar cada 2 s
        if (!fs_ok && now - last_rescan >= 2000) {
            last_rescan = now;
            if (sd_spi_init()) {
                if (tree_scan()) {
                    printf("SD detectada: %s\r\n", sd_spi_type_name());
                    usb_msc_init();
                    tree_reset();
                    fs_ok = true;
                }
            }
            continue;
        }

        // Re-scan periodico (fuera de trafico USB)
        if (fs_ok && now - last_rescan >= FB_RESCAN_MS) {
            last_rescan = now;
            if (!g_usb_state.reading && !g_usb_state.writing) {
                tree_scan();
                tree_render();
            }
        }

        tree_tick(now);

        if (g_usb_state.mounted) {
            led_set(true);
        } else if (now - last_blink >= 500) {
            last_blink = now;
            static bool led_state = false;
            led_state = !led_state;
            led_set(led_state);
        }
    }
}
