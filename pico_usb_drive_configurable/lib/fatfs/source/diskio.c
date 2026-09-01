/*-----------------------------------------------------------------------/
/  diskio.c - Low level disk interface for the internal W25Q16 flash      /
/  Overrides the SD/MMC sample from FatFS.                                /
/                                                                         /
/  Maps the FAT filesystem to a region of the RP2040's onboard 2 MB       /
/  flash (W25Q16) starting at OFFSET_EN_FLASH. Uses the pico-sdk XIP-safe /
/  flash_range_erase() / flash_range_program() to persist data.          /
/                                                                         /
/  (c) ChaN - original FatFS diskio module                                /
/  SPDX-License-Identifier: MIT                                           /
/-----------------------------------------------------------------------*/

#include "ff.h"          /* Obtains integer types */
#include "diskio.h"      /* Declarations of disk functions */

#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "config.h"

/* Physical address base for our FAT area inside the 2 MB flash. */
#define FLASH_BASE        (OFFSET_EN_FLASH)
/* Size of the whole exposed disk in bytes. */
#define FLASH_SIZE        (TAMAÑO_MAXIMO_EN_BYTES)
/* W25Q16 erase sector size = 4096 bytes. */
#define FLASH_ERASE_SIZE  4096u

/*--------------------------------------------------------------------------

   Public Functions (see diskio.h for descriptions)

---------------------------------------------------------------------------*/

/* 
 * disk_initialize - Prepare the physical medium.
 * The flash is always present; nothing to do beyond clearing the "NOINIT"
 * flag. Called by f_mount().
 */
DSTATUS disk_initialize(BYTE pdrv) {
    (void) pdrv;
    return 0; /* no init error, drive ready */
}

/*
 * disk_status - Return the current status of the drive.
 * Always ready (no medium-removal, no hardware write-protect switch).
 */
DSTATUS disk_status(BYTE pdrv) {
    (void) pdrv;
    return 0; /* STA_NOINIT clear, STA_PROTECT clear -> ready & writable */
}

/*
 * disk_read - Read sector(s) from flash.
 * The pico-sdk XIP (execute in place) lets us read flash directly with memcpy
 * while code runs from flash, so a plain copy is safe and fast.
 */
DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    (void) pdrv;
    if (sector + count > (LBA_t) (FLASH_SIZE / DISK_SECTOR_SIZE)) {
        return RES_PARERR;
    }
    const uint8_t *src = (const uint8_t *) (XIP_BASE + FLASH_BASE + (sector * DISK_SECTOR_SIZE));
    memcpy(buff, src, (size_t) count * DISK_SECTOR_SIZE);
    return RES_OK;
}

/*
 * disk_write - Write sector(s) to flash.
 * Buttons: Erase granularity is 4096 B, write granularity is 256 B. To keep
 * it simple and correct we read-modify-erase-write whole 4 KB erase blocks.
 * NOTE: flash_range_program() disables interrupts and uses XIP-safe code;
 * temporarily blocks the USB stack (acceptable for small writes).
 */
DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    (void) pdrv;
    if (sector + count > (LBA_t) (FLASH_SIZE / DISK_SECTOR_SIZE)) {
        return RES_WRPRT;
    }

    /* small internal buffer must be in normal RAM (not flash XIP) */
    static uint8_t erase_buf[FLASH_ERASE_SIZE];
    /* process one full erase block at a time */
    while (count > 0) {
        /* absolute flash offset of the first sector of this erase block */
        uint32_t abs_offs = FLASH_BASE + (uint32_t) sector * DISK_SECTOR_SIZE;
        uint32_t block_start = abs_offs & ~(FLASH_ERASE_SIZE - 1u);
        uint32_t offset_in_block = abs_offs - block_start;

        /* read the current block content (it may be not fully rewritten) */
        memcpy(erase_buf, (const uint8_t *) (XIP_BASE + block_start), FLASH_ERASE_SIZE);

        /* overlay the sectors we must update */
        uint32_t to_write = FLASH_ERASE_SIZE - offset_in_block; /* bytes left in block */
        uint32_t want = (uint32_t) count * DISK_SECTOR_SIZE;
        if (want < to_write) to_write = want;
        memcpy(erase_buf + offset_in_block, buff, to_write);

        /* erase + program the whole 4 KB block */
        uint32_t saved = save_and_disable_interrupts();
        flash_range_erase(block_start, FLASH_ERASE_SIZE);
        flash_range_program(block_start, erase_buf, FLASH_ERASE_SIZE);
        restore_interrupts(saved);

        /* advance */
        uint32_t sectors_done = to_write / DISK_SECTOR_SIZE;
        sector += sectors_done;
        count  -= sectors_done;
        buff    += to_write;
    }
    return RES_OK;
}

/*
 * disk_ioctl - Control device dependent features.
 * We support CTRL_SYNC (flush) and GET_SECTOR_COUNT which are the ones the
 * FAT layer actually needs. flash writes are synchronous already.
 */
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    (void) pdrv;
    switch (cmd) {
        case CTRL_SYNC:
            /* flash_range_program returns after bytes are committed -> synced */
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(LBA_t*) buff = (LBA_t) (FLASH_SIZE / DISK_SECTOR_SIZE);
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD*) buff = DISK_SECTOR_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD*) buff = FLASH_ERASE_SIZE / DISK_SECTOR_SIZE;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}
