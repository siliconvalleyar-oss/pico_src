/*
 * msc_disk.h - RAM disk / Mass Storage Class backend for pendrive
 *
 * Exposes a configurable in-RAM FAT disk that the host sees as a USB
 * pendrive. It is fully writable (read10/write10) and formatted on boot
 * as FAT12 or FAT16 depending on the configured size.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _MSC_DISK_H_
#define _MSC_DISK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// CONFIGURATION
//--------------------------------------------------------------------+

// Block (sector) size of the disk. USB MSC / FAT work with 512 bytes.
#define PENDISK_BLOCK_SIZE        512u

// CONFIGURABLE CAPACITY: number of 512-byte blocks that make up the disk.
// The RP2040 has 264 KB of SRAM, so this RAM disk is limited by memory.
//
//   blocks  -->  capacity
//   256     -->  128 KB   (default)
//   512     -->  256 KB
//   128     -->  64 KB
//   384     -->  192 KB   (do not exceed ~ 480 blocks on a stock Pico)
//
// Changing this value only redefines the disk size. The filesystem is
// formatted automatically on boot, picking FAT12 or FAT16 as appropriate.
#ifndef PENDISK_BLOCK_COUNT
#define PENDISK_BLOCK_COUNT        256u
#endif

// Sectors per cluster of the FAT filesystem. 1 is fine for these sizes and
// keeps wasted space to a minimum. 2 or 4 can be used for larger disks.
#ifndef PENDISK_SECTORS_PER_CLUSTER
#define PENDISK_SECTORS_PER_CLUSTER 1u
#endif

// Lightweight LED blink while the host is writing (0 = disabled). Uses
// PICO_DEFAULT_LED_PIN when available.
#ifndef PENDISK_ACTIVITY_LED
#define PENDISK_ACTIVITY_LED       1
#endif

//--------------------------------------------------------------------+
// PUBLIC API
//--------------------------------------------------------------------+

// Format the RAM disk with a fresh FAT filesystem, including a default
// README file. Called automatically at boot.
void pendisk_format(void);

// Total number of blocks on the disk (== PENDISK_BLOCK_COUNT).
uint32_t pendisk_block_count(void);

// Whether the disk has been ejected (safe removed) by the host.
bool pendisk_is_ejected(void);

// Mark disk as ejected/loaded (used by MSC start/stop callback).
void pendisk_set_ejected(bool ejected);

#ifdef __cplusplus
}
#endif

#endif /* _MSC_DISK_H_ */
