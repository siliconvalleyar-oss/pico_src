/*
 * file_browser.h - Escaneo del directorio raiz de la microSD
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MICRO_SD_FILE_BROWSER_H
#define MICRO_SD_FILE_BROWSER_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"   // FB_MAX_FILES, FB_NAME_LEN

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char     name[64];  // nombre LFN (truncado a FB_NAME_LEN al mostrar)
    bool     is_dir;
    uint32_t size;      // bytes (si es archivo)
} fb_entry_t;

typedef struct {
    fb_entry_t  files[FB_MAX_FILES];
    uint16_t    count;          // entradas validas en files[]
    bool        valid;          // hay un escaneo vigente
} fb_list_t;

/* Escanea "/" de la microSD (ya montada). Devuelve false ante error.
 * Guarda el resultado en la lista interna estatica (thread-unsafe: single loop). */
bool fb_scan(void);

/* Acceso a la lista escaneada (solo lectura). */
const fb_list_t *fb_list(void);

/* Nombre del volumen FAT ("MICROSD " si no tiene). */
const char *fb_volume_label(void);

/* Espacio libre/total en MB. */
void fb_free_mb(uint32_t *free_mb, uint32_t *total_mb);

#ifdef __cplusplus
}
#endif

#endif // MICRO_SD_FILE_BROWSER_H
