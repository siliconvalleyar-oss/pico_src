/*
 * tree_view.h - Vista tipo "tree ." del contenido de la microSD
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Recorre el FAT en profundidad (carpetas primero) y aplana el arbol a una
 * lista de lineas con prefijos de rama, lista para dibujar en el OLED.
 *
 * Limitado por TREE_MAX_NODES y TREE_MAX_DEPTH para acotar RAM y tiempo de
 * scan (el OLED solo muestra TREE_VIEW_ROWS lineas por vez).
 */

#ifndef MICRO_SD_TREE_VIEW_H
#define MICRO_SD_TREE_VIEW_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Lineas maximas del arbol aplanado (6 filas x ~10 pantallas de scroll)
#define TREE_MAX_NODES      60
#define TREE_MAX_DEPTH      2      // raiz + 2 niveles de carpetas
#define TREE_NAME_MAX       20     // chars de nombre por linea (con prefijo)

typedef struct {
    char     text[TREE_NAME_MAX + 8];  // "  +-NOMBRE/" + '\0' (ya en MAYUSCULAS)
    bool     is_dir;
    bool     is_last;                  // ultimo hijo de su padre (para '+-'/\'-')
    uint32_t size;                     // bytes (0 en carpetas)
} tree_node_t;

typedef struct {
    tree_node_t nodes[TREE_MAX_NODES];
    uint16_t    count;                 // nodos validos
    bool        valid;                 // scan vigente
    bool        truncated;             // se corto por TREE_MAX_NODES/DEPTH
} tree_t;

/* Recorre la raiz "/" en profundidad y aplana el arbol.
 * Devuelve false si la SD/FAT no esta montada. */
bool tree_scan(void);

/* Acceso al arbol actual (solo lectura). */
const tree_t *tree_get(void);

/* Nombre del volumen FAT (para la cabecera). */
const char *tree_volume_label(void);

#ifdef __cplusplus
}
#endif

#endif // MICRO_SD_TREE_VIEW_H
