/*
 * sd_spi.h - Driver de microSD en modo SPI (protocolo SD) para RP2040
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Implementa la secuencia de inicialización del estándar SD:
 *   74+ clocks con CS alto -> CMD0 -> CMD8 -> ACMD41 -> CMD59 -> CMD16
 * y lecturas/escrituras de sectores de 512 bytes (CMD17/CMD24).
 *
 * Soporta MMC, SDv1, SDv2 y SDHC/SDXC (direccionamiento por bloque).
 */

#ifndef MICRO_SD_SD_SPI_H
#define MICRO_SD_SD_SPI_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SD_TYPE_UNKNOWN = 0,
    SD_TYPE_MMC,        // MMC v3 (solo lecturas/escrituras básicas)
    SD_TYPE_V1,         // SD v1 (direccionamiento por byte)
    SD_TYPE_V2,         // SD v2 (direccionamiento por byte)
    SD_TYPE_V2_HC       // SDHC/SDXC (direccionamiento por bloque de 512)
} sd_type_t;

/* Inicializa SPI1 + pines y la tarjeta. Devuelve false si no hay tarjeta. */
bool sd_spi_init(void);

/* Tipo de tarjeta detectado (válido tras sd_spi_init()). */
sd_type_t sd_spi_type(void);

/* Nombre corto del tipo ("MMC","SD1","SD2","SDHC","?"). */
const char *sd_spi_type_name(void);

/* Cantidad total de sectores de 512 bytes (a partir del CSD). */
uint32_t sd_spi_sector_count(void);

/* Idem, con cache (para callbacks USB repetitivos). */
uint32_t sd_sector_count_cached(void);

/* Lee n sectores a partir de lba. Devuelve false ante error/timeout. */
bool sd_spi_read_sectors(uint32_t lba, uint8_t *buf, uint32_t n);

/* Escribe n sectores a partir de lba. Devuelve false ante error/timeout. */
bool sd_spi_write_sectors(uint32_t lba, const uint8_t *buf, uint32_t n);

/* Vuelve al estado "no inicializado" (para reintentos al cambiar tarjeta). */
void sd_spi_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // MICRO_SD_SD_SPI_H
