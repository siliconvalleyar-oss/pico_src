/*
 * tree_view.c - Recorrido recursivo del FAT (tipo "tree .") aplanado a lineas
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Estrategia: DFS con f_opendir/f_readdir por carpeta. Carpetas primero,
 * alfabetico en cada nivel (mismo criterio que la vista ls). Cada nodo
 * genera UNA linea de texto con prefijo de rama ("  +-", "  '-", "|  ").
 * La recursion se acota con TREE_MAX_DEPTH y TREE_MAX_NODES para que el
 * peor caso (tarjeta llena de carpetas) no desborde RAM ni el tiempo de
 * re-escaneo periodico.
 *
 * Nota: la pila de recursion lleva una copia de f_readdir state por nivel
 * (DIR ~ 550 B en R0.15a con LFN): 2 niveles ~ 1.1 KB, aceptable.
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "ff.h"

#include "config.h"
#include "tree_view.h"

static tree_t s_tree;
static char   s_label[16];

// Estado de la recursion
typedef struct {
    uint16_t *count;        // puntero al contador global de nodos
    bool     *truncated;
} walk_ctx_t;

// Orden: carpetas primero; alfabetico dentro de cada grupo
static int cmp_fno(const FILINFO *a, const FILINFO *b) {
    bool da = (a->fattrib & AM_DIR) != 0;
    bool db = (b->fattrib & AM_DIR) != 0;
    if (da != db) return da ? -1 : 1;
    return strcasecmp(a->fname, b->fname);
}

/*
 * Aplana la carpeta 'path' al arbol. 'prefix' es la rama acumulada
 * ("", "| ", "  ", ...) y 'depth' el nivel actual (0 = hijos de la raiz).
 */
static void walk_dir(const char *path, const char *prefix, int depth, walk_ctx_t *ctx) {
    DIR dir;
    FILINFO fno;

    if (f_opendir(&dir, path) != FR_OK) return;

    // Recolectar las entradas de ESTE nivel y ordenarlas
    // (max 32 por carpeta: suficiente para el display y acota RAM)
    FILINFO entries[32];
    uint16_t n = 0;
    while (n < 32) {
        if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == '\0') break;
        if (fno.fattrib & AM_HID) continue;
        entries[n++] = fno;
    }
    f_closedir(&dir);

    // insertion sort (n pequeno)
    for (uint16_t i = 1; i < n; i++) {
        FILINFO key = entries[i];
        int16_t j = (int16_t) i - 1;
        while (j >= 0 && cmp_fno(&entries[j], &key) > 0) {
            entries[j + 1] = entries[j];
            j--;
        }
        entries[j + 1] = key;
    }

    for (uint16_t i = 0; i < n; i++) {
        if (*ctx->count >= TREE_MAX_NODES) { *ctx->truncated = true; return; }

        FILINFO *e = &entries[i];
        bool is_dir = (e->fattrib & AM_DIR) != 0;
        bool last = (i == n - 1);

        tree_node_t *node = &s_tree.nodes[(*ctx->count)++];

        // Linea: prefix + ("+-" o "'-") + nombre + ("/" si carpeta)
        // La fuente 8x8 cubre a-z/A-Z/0-9: se respeta el caso original
        snprintf(node->text, sizeof(node->text), "%s%s%.16s%s",
                 prefix,
                 last ? "'-" : "+-",
                 e->fname,
                 is_dir ? "/" : "");
        node->is_dir  = is_dir;
        node->is_last = last;
        node->size    = is_dir ? 0 : (uint32_t) e->fsize;

        // Recursion en carpetas (si hay presupuesto de profundidad)
        if (is_dir && depth + 1 < TREE_MAX_DEPTH && *ctx->count < TREE_MAX_NODES) {
            char subpath[64];
            snprintf(subpath, sizeof(subpath), "%s/%s", path, e->fname);

            // Prefijo del hijo: hereda la linea vertical del padre
            char subprefix[TREE_NAME_MAX];
            snprintf(subprefix, sizeof(subprefix), "%s%s",
                     prefix, last ? "  " : "| ");

            walk_dir(subpath, subprefix, depth + 1, ctx);
            if (*ctx->truncated) return;
        }
    }
}

bool tree_scan(void) {
    static FATFS fs;

    memset(&s_tree, 0, sizeof(s_tree));

    if (f_mount(&fs, "", 1) != FR_OK) {
        return false;
    }

#if FF_USE_LABEL
    TCHAR lbl[32];
    DWORD vsn;
    if (f_getlabel("", lbl, &vsn) == FR_OK && lbl[0] != '\0') {
        strncpy(s_label, lbl, sizeof(s_label) - 1);
        s_label[sizeof(s_label) - 1] = '\0';
    }
#endif

    // Primera linea: la raiz misma, como "tree ." muestra "."
    tree_node_t *root = &s_tree.nodes[0];
    snprintf(root->text, sizeof(root->text), ".");
    root->is_dir  = true;
    root->is_last = false;
    root->size    = 0;
    s_tree.count  = 1;

    walk_ctx_t ctx = { .count = &s_tree.count, .truncated = &s_tree.truncated };
    walk_dir("/", "", 0, &ctx);

    s_tree.valid = true;
    return true;
}

const tree_t *tree_get(void) {
    return &s_tree;
}

const char *tree_volume_label(void) {
    return s_label[0] ? s_label : "MICROSD";
}
