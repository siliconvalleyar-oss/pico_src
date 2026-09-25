/*
 * main.cpp - Pico + microSD (SPI1) + OLED SSD1306 (I2C1) + USB MSC
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Hardware:
 *   microSD -> SPI1: SCK=GP10, MOSI=GP11, MISO=GP12, CS=GP13
 *   OLED    -> I2C1: SDA=GP2,  SCL=GP3  (SSD1306 128x64 @ 0x3C)
 *   USB     -> Mass Storage: la PC ve el FAT de la microSD como pendrive
 *
 * Comportamiento:
 *   1. Al arrancar muestra "MICRO-SD" mientras inicializa la tarjeta.
 *   2. Si detecta la tarjeta, monta el FAT y llena la pantalla con el banner
 *      de archivos: se desliza hacia abajo y al pasar el ultimo vuelve al
 *      primero (scroll infinito). Cada FB_RESCAN_MS re-escanea por cambios.
 *   3. Si no hay tarjeta muestra "NO SD" y reintenta cada 2 s.
 *   4. En paralelo, el USB expone la tarjeta como pendrive (MSC). La tarea
 *      USB se bombea entre refrescos del OLED (loop cooperativo).
 *
 * El LED onboard parpadea mientras el USB esta desconectado y queda fijo
 * cuando el host monta el disco.
 */

#include <cstdio>
#include <cstring>
#include <cctype>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"   // LED onboard del Pico W (via CYW43)
#endif

extern "C" {
#include "ssd1306.h"
#include "sd_spi.h"
#include "file_browser.h"
#include "usb_msc.h"
#if SD_AUTO_FORMAT
#include "ff.h"
#endif
}

#include "config.h"

//--------------------------------------------------------------------+
// OLED (usado por el banner)
//--------------------------------------------------------------------+
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
    // Pico W: el LED va al WiFi chip; requiere init del CYW43
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
// Banner deslizante de archivos
//--------------------------------------------------------------------+

// Estados del scroll
static uint16_t       s_top   = 0;   // indice del primer archivo visible
static uint32_t       s_last_step_ms = 0;

// Dibuja el banner: titulo, tipo de tarjeta y BANNER_ROWS archivos.
// La ventana visible arranca en s_top y avanza ciclicamente.
static void banner_render(void) {
    const fb_list_t *list = fb_list();

    ssd1306_clear(&oled);

    // Fila 0: etiqueta del volumen
    char line[17];
    snprintf(line, sizeof(line), "%s", fb_volume_label());
    ssd1306_draw_string(&oled, 0, 0, line);

    // Fila 1 (izq): tipo de tarjeta; (der): cantidad de archivos
    snprintf(line, sizeof(line), "%s", sd_spi_type_name());
    ssd1306_draw_string(&oled, 0, 8, line);
    snprintf(line, sizeof(line), "N=%u", list->count);
    ssd1306_draw_string(&oled, 88, 8, line);

    ssd1306_draw_line(&oled, 0, 16, 127, 16, true);

    if (list->count == 0) {
        ssd1306_draw_string(&oled, 0, 28, "VACIO");
        ssd1306_update(&oled);
        return;
    }

    // Ventana de archivos: cicla desde s_top hacia abajo
    for (int row = 0; row < BANNER_ROWS; row++) {
        uint16_t idx = (uint16_t) ((s_top + row) % list->count);
        const fb_entry_t *e = &list->files[idx];

        char name[FB_NAME_LEN + 1];
        strncpy(name, e->name, FB_NAME_LEN);
        name[FB_NAME_LEN] = '\0';
        // La fuente 8x8 cubre a-z/A-Z/0-9: se respeta el caso original

        int y = 17 + row * 8;

        if (e->is_dir) {
            // Carpeta: prefijo "/" + nombre (sin tamaño)
            char dline[FB_NAME_LEN + 2];
            snprintf(dline, sizeof(dline), "/%s", name);
            ssd1306_draw_string(&oled, 0, y, dline);
        } else {
            // Archivo: nombre a la izquierda + tamaño legible a la derecha
            char size[8];
            if      (e->size >= 1024u * 1024u) snprintf(size, sizeof(size), "%luM", (unsigned long) (e->size / (1024u * 1024u)));
            else if (e->size >= 1024u)         snprintf(size, sizeof(size), "%luK", (unsigned long) (e->size / 1024u));
            else                               snprintf(size, sizeof(size), "%lu",  (unsigned long) e->size);

            // Recortar el nombre para que no pise la columna de tamaño
            // (2 chars de tamaño + 1 espacio => nombre max 13 chars)
            char trunc[FB_NAME_LEN + 1];
            snprintf(trunc, sizeof(trunc), "%-13.13s", name);

            char line[24];
            snprintf(line, sizeof(line), "%s%4s", trunc, size);
            ssd1306_draw_string(&oled, 0, y, line);
        }
    }

    ssd1306_update(&oled);
}

// Avanza el scroll: desliza una fila y al pasar la ultima vuelve a la primera
static void banner_tick(uint32_t now_ms) {
    if (s_last_step_ms == 0) s_last_step_ms = now_ms;

    if (now_ms - s_last_step_ms >= BANNER_SCROLL_MS) {
        s_last_step_ms = now_ms;
        const fb_list_t *list = fb_list();
        if (list->count > 0) {
            s_top = (uint16_t) ((s_top + 1) % list->count);
        }
        banner_render();
    }
}

static void banner_reset(void) {
    s_top = 0;
    s_last_step_ms = 0;
    banner_render();
}

//--------------------------------------------------------------------+
// Punto de entrada
//--------------------------------------------------------------------+
int main() {
    stdio_init_all();
    led_init();
    led_set(true);

    printf("\r\n=== PICO + microSD + OLED + USB MSC ===\r\n");

    // OLED primero: da feedback inmediato
    ssd1306_init(&oled, OLED_I2C_INST, OLED_SDA_PIN, OLED_SCL_PIN);
    ssd1306_clear(&oled);
    ssd1306_draw_string(&oled, 0, 0, "MICRO-SD");
    ssd1306_draw_line(&oled, 0, 16, 127, 16, true);
    ssd1306_draw_string(&oled, 0, 24, "INICIANDO...");
    ssd1306_update(&oled);

    bool sd_ok = sd_spi_init();
    bool fs_ok = false;

    if (sd_ok) {
        printf("SD: %s, %u sectores\r\n", sd_spi_type_name(),
               (unsigned) sd_sector_count_cached());
#if SD_AUTO_FORMAT
        fs_ok = fb_scan();
        if (!fs_ok) {
            // Formatear solo si SD_AUTO_FORMAT=1 (destructivo)
            static FATFS fs;
            static uint8_t work[4096];
            MKFS_PARM opt;
            memset(&opt, 0, sizeof(opt));
            opt.fmt = FM_FAT32;
            if (f_mkfs("", &opt, work, sizeof(work)) == FR_OK) {
                fs_ok = fb_scan();
            }
        }
#else
        fs_ok = fb_scan();
#endif
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
        uint32_t free_mb = 0, total_mb = 0;
        fb_free_mb(&free_mb, &total_mb);
        printf("FAT: %s - %u archivos, %lu/%lu MB libres\r\n",
               fb_volume_label(), fb_list()->count,
               (unsigned long) free_mb, (unsigned long) total_mb);
        banner_reset();
    }

    // USB MSC: exponer la tarjeta como pendrive
    usb_msc_init();
    printf("USB MSC: listo\r\n");

    uint32_t last_rescan = to_ms_since_boot(get_absolute_time());
    uint32_t last_blink  = last_rescan;

    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // USB siempre atento (loop cooperativo con el resto)
        usb_msc_task();

        // Sin tarjeta: reintentar inicializacion cada 2 s
        if (!fs_ok && now - last_rescan >= 2000) {
            last_rescan = now;
            if (sd_spi_init()) {
                fs_ok = fb_scan();
                if (fs_ok) {
                    uint32_t free_mb = 0, total_mb = 0;
                    fb_free_mb(&free_mb, &total_mb);
                    printf("SD detectada: %s, %u archivos\r\n",
                           sd_spi_type_name(), fb_list()->count);
                    (void) free_mb; (void) total_mb;
                    usb_msc_init();  // re-enumerar el disco nuevo
                    banner_reset();
                }
            }
            continue;
        }

        // Re-escaneo periodico del directorio (solo si no hay trafico USB)
        if (fs_ok && now - last_rescan >= FB_RESCAN_MS) {
            last_rescan = now;
            if (!g_usb_state.reading && !g_usb_state.writing) {
                fb_scan();
                s_top = 0;
                banner_render();
            }
        }

        // Banner deslizante
        banner_tick(now);

        // LED: fijo si USB montado; parpadea si no
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
