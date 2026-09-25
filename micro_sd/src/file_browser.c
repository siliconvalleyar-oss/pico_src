/*
 * file_browser.c - Escaneo del directorio raiz de la microSD con FatFS
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "ff.h"

#include "config.h"
#include "file_browser.h"

static fb_list_t  s_list;
static char       s_label[16];

// Carpetas primero, dentro de cada grupo alfabético (case-insensitive)
static int cmp_entries(const void *a, const void *b) {
    const fb_entry_t *ea = (const fb_entry_t *) a;
    const fb_entry_t *eb = (const fb_entry_t *) b;
    if (ea->is_dir != eb->is_dir) return ea->is_dir ? -1 : 1;
    return strcasecmp(ea->name, eb->name);
}

bool fb_scan(void) {
    static FATFS fs;      // objeto de volumen (persistente)
    DIR dir;
    FILINFO fno;

    memset(&s_list, 0, sizeof(s_list));

    // Montar (deferred: monta en el primer acceso)
    if (f_mount(&fs, "", 1) != FR_OK) {
        return false;
    }

    // Etiqueta del volumen (opcional)
    s_label[0] = '\0';
#if FF_USE_LABEL
    TCHAR lbl[32];
    DWORD vsn;
    if (f_getlabel("", lbl, &vsn) == FR_OK && lbl[0] != '\0') {
        strncpy(s_label, lbl, sizeof(s_label) - 1);
        s_label[sizeof(s_label) - 1] = '\0';
    }
#endif

    if (f_opendir(&dir, "/") != FR_OK) {
        return false;
    }

    uint16_t n = 0;
    while (n < FB_MAX_FILES) {
        if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == '\0') break;

        // Ignorar entradas de volumen y ocultas
        if (fno.fattrib & (AM_HID)) continue;

        fb_entry_t *e = &s_list.files[n];
        strncpy(e->name, fno.fname, sizeof(e->name) - 1);
        e->name[sizeof(e->name) - 1] = '\0';
        e->is_dir = (fno.fattrib & AM_DIR) != 0;
        e->size   = e->is_dir ? 0 : (uint32_t) fno.fsize;
        n++;
    }
    f_closedir(&dir);

    // Orden tipo "ls": carpetas primero, luego archivos, alfabético
    // (FatFS readdir devuelve las entradas en orden de asignación)
    qsort(s_list.files, n, sizeof(fb_entry_t), cmp_entries);

    s_list.count = n;
    s_list.valid = true;
    return true;
}

const fb_list_t *fb_list(void) {
    return &s_list;
}

const char *fb_volume_label(void) {
    return s_label[0] ? s_label : "MICROSD";
}

void fb_free_mb(uint32_t *free_mb, uint32_t *total_mb) {
    static FATFS fs;
    FATFS *fsp;
    DWORD fre_clst;

    if (f_getfree("", &fre_clst, &fsp) != FR_OK) {
        if (free_mb)  *free_mb = 0;
        if (total_mb) *total_mb = 0;
        return;
    }
    uint64_t total = (uint64_t) (fsp->n_fatent - 2u) * fsp->csize * SD_SECTOR_SIZE;
    uint64_t freeb = (uint64_t) fre_clst * fsp->csize * SD_SECTOR_SIZE;
    if (total_mb)  *total_mb = (uint32_t) (total / (1024u * 1024u));
    if (free_mb)   *free_mb  = (uint32_t) (freeb / (1024u * 1024u));
}
