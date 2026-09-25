/*
 * sd_diskio.c - Interface diskio de FatFS contra la microSD (SPI)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * FatFS llama aqui; cada funcion delega en sd_spi.c.
 * Solo hay un volumen (pdrv=0).
 */

#include "ff.h"
#include "ffconf.h"
#include "diskio.h"

#include "pico/stdlib.h"

#include "config.h"
#include "sd_spi.h"

static DSTATUS s_stat = STA_NOINIT;

// Fecha/hora fija (no hay RTC): 1/1/2022 (coherente con FF_NORTC en ffconf.h)
DWORD get_fattime(void) {
    return ((DWORD) (2022 - 1980) << 25) |
           ((DWORD) 1 << 21) |
           ((DWORD) 1 << 16);
}

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;

    if (sd_spi_init()) {
        s_stat &= (DSTATUS) ~STA_NOINIT;
    } else {
        s_stat |= STA_NOINIT;
    }
    return s_stat;
}

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return s_stat;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || count == 0) return RES_PARERR;
    if (s_stat & STA_NOINIT) return RES_NOTRDY;

    return sd_spi_read_sectors((uint32_t) sector, buff, count) ? RES_OK : RES_ERROR;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || count == 0) return RES_PARERR;
    if (s_stat & STA_NOINIT) return RES_NOTRDY;

    return sd_spi_write_sectors((uint32_t) sector, buff, count) ? RES_OK : RES_ERROR;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    if (pdrv != 0) return RES_PARERR;
    if (s_stat & STA_NOINIT) return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC:
            // Las escrituras SPI son sincronas: nada pendiente.
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(LBA_t *) buff = (LBA_t) sd_spi_sector_count();
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD *) buff = SD_SECTOR_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            // Bloque de borrado en sectores (para f_mkfs); 8 x 512 = 4 KB.
            *(DWORD *) buff = 8;
            return RES_OK;

        case MMC_GET_TYPE:
            *(BYTE *) buff = (BYTE) sd_spi_type();
            return RES_OK;

        default:
            return RES_PARERR;
    }
}
