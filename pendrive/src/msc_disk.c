/*
 * msc_disk.c - Configurable RAM disk presented as a USB pendrive
 *
 * Implements a fully writable FAT12/FAT16 disk held in the Pico's SRAM and
 * the TinyUSB MSC callbacks that make it look like a normal USB pendrive.
 *
 * The filesystem is formatted in RAM at boot, so capacity is configurable
 * (see PENDISK_BLOCK_COUNT in msc_disk.h) and the disk is writable like any
 * real pendrive. Because the medium is volatile RAM, the contents are lost
 * on power-off -- the definition of a ramdrive.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include <stdlib.h>
#include "bsp/board_api.h"
#include "tusb.h"
#include "hardware/gpio.h"
#include "msc_disk.h"

//--------------------------------------------------------------------+
// RAM DISK BUFFER
//--------------------------------------------------------------------+

// The whole disk lives here. --- Not const --- because it is writable.
static uint8_t msc_disk[PENDISK_BLOCK_COUNT][PENDISK_BLOCK_SIZE];

// Whether the host did a safe-eject (so the disk should not be written).
static bool ejected = false;

uint32_t pendisk_block_count(void) {
    return PENDISK_BLOCK_COUNT;
}

bool pendisk_is_ejected(void) {
    return ejected;
}

void pendisk_set_ejected(bool e) {
    ejected = e;
}

//--------------------------------------------------------------------+
// FAT GEOMETRY
//--------------------------------------------------------------------+

enum {
    FAT_TYPE_12 = 12,
    FAT_TYPE_16 = 16,
};

typedef struct {
    uint8_t  fat_type;          // FAT12 or FAT16
    uint16_t bytes_per_sector;  // always 512
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;
    uint32_t total_sectors;
    uint16_t sectors_per_fat;
    uint32_t root_dir_sectors;
    uint32_t first_fat_sector;
    uint32_t first_root_sector;
    uint32_t first_data_sector;
    uint32_t cluster_count;
} fat_geom_t;

static fat_geom_t geom;

// Number of sectors occupied by the root directory.
static uint32_t root_dir_sectors(uint16_t root_entries) {
    return ((uint32_t) root_entries * 32u + PENDISK_BLOCK_SIZE - 1u) / PENDISK_BLOCK_SIZE;
}

static void compute_geometry(void) {
    geom.bytes_per_sector     = PENDISK_BLOCK_SIZE;
    geom.sectors_per_cluster  = PENDISK_SECTORS_PER_CLUSTER;
    geom.reserved_sectors     = 4;                 // reserved (boot + some slack)
    geom.num_fats             = 2;
    geom.root_entries         = 512;
    geom.total_sectors        = PENDISK_BLOCK_COUNT;
    geom.root_dir_sectors     = root_dir_sectors(geom.root_entries);

    // Initial estimate of FAT size (Microsoft FAT spec formula).
    uint32_t tmp1 = geom.total_sectors - (geom.reserved_sectors + geom.root_dir_sectors);
    uint32_t tmp2 = (256u * geom.sectors_per_cluster) + geom.num_fats;
    uint32_t est  = (tmp1 + tmp2 - 1u) / tmp2;
    if (est < 1u) est = 1u;

    // Iterate until the FAT size is stable (depends on cluster count).
    for (int i = 0; i < 8; i++) {
        uint32_t tmp3 = geom.reserved_sectors + (geom.num_fats * est) + geom.root_dir_sectors;
        uint32_t clusters = (geom.total_sectors - tmp3) / geom.sectors_per_cluster;

        uint32_t new_fat;
        if (clusters < 4085u) {
            // FAT12: 1.5 bytes per entry
            new_fat = (((clusters + 2u) * 3u / 2u) + PENDISK_BLOCK_SIZE - 1u) / PENDISK_BLOCK_SIZE;
        } else {
            // FAT16: 2 bytes per entry
            new_fat = (((clusters + 2u) * 2u) + PENDISK_BLOCK_SIZE - 1u) / PENDISK_BLOCK_SIZE;
        }
        if (new_fat < 1u) new_fat = 1u;

        if (new_fat == est) {
            est = new_fat;
            break;
        }
        est = new_fat;
    }

    geom.sectors_per_fat = (uint16_t) est;

    // Recompute cluster count with the final FAT size.
    uint32_t used = geom.reserved_sectors + (geom.num_fats * geom.sectors_per_fat) + geom.root_dir_sectors;
    uint32_t clusters = (geom.total_sectors - used) / geom.sectors_per_cluster;

    geom.first_fat_sector   = geom.reserved_sectors;
    geom.first_root_sector  = geom.first_fat_sector + (geom.num_fats * geom.sectors_per_fat);
    geom.first_data_sector  = geom.first_root_sector + geom.root_dir_sectors;

    // Data clusters that are actually addressable.
    uint32_t max_data_clusters = (geom.total_sectors - geom.first_data_sector) / geom.sectors_per_cluster;
    geom.cluster_count = (clusters < max_data_clusters) ? clusters : max_data_clusters;
    if (geom.cluster_count < 2u) geom.cluster_count = 2u;

    geom.fat_type = (geom.cluster_count < 4085u) ? FAT_TYPE_12 : FAT_TYPE_16;
}

//--------------------------------------------------------------------+
// SECTOR READ / WRITE HELPERS
//--------------------------------------------------------------------+

static void disk_clear(void) {
    memset(msc_disk, 0, sizeof(msc_disk));
}

static void wipe_sector(uint32_t sector) {
    memset(msc_disk[sector], 0, PENDISK_BLOCK_SIZE);
}

static void read_fat(uint8_t *clusters, uint32_t cluster) {
    if (geom.fat_type == FAT_TYPE_16) {
        // Each FAT entry is 2 bytes.
        uint32_t offset = cluster * 2u;
        uint32_t sector = geom.first_fat_sector + (offset / PENDISK_BLOCK_SIZE);
        uint32_t insec  = offset % PENDISK_BLOCK_SIZE;
        uint8_t  hi, lo;
        if (insec + 1u < PENDISK_BLOCK_SIZE) {
            hi = msc_disk[sector][insec];
            lo = msc_disk[sector][insec + 1u];
        } else {
            // crossing a sector boundary (shouldn't happen with our sizes)
            uint8_t tmp[2] = {0, 0};
            tmp[0] = msc_disk[sector][insec];
            tmp[1] = msc_disk[sector + 1u][0];
            hi = tmp[0]; lo = tmp[1];
        }
        clusters[0] = hi;
        clusters[1] = lo;
    } else {
        // FAT12: 1.5 bytes per entry.
        uint32_t offset = cluster + (cluster / 2u);
        uint32_t sector = geom.first_fat_sector + (offset / PENDISK_BLOCK_SIZE);
        uint32_t insec  = offset % PENDISK_BLOCK_SIZE;

        uint8_t b0 = msc_disk[sector][insec];
        uint8_t b1;
        if (insec + 1u < PENDISK_BLOCK_SIZE) {
            b1 = msc_disk[sector][insec + 1u];
        } else {
            b1 = msc_disk[sector + 1u][0];
        }

        uint16_t both;
        if (cluster & 1u) {
            both = ((uint16_t) b1 << 4) | (b0 >> 4);
        } else {
            both = ((uint16_t) b1 << 8) | b0;
        }
        clusters[0] = (uint8_t) (both & 0xFFu);
        clusters[1] = (uint8_t) ((both >> 8) & 0xFFu);
    }
}

static uint16_t get_fat_entry(uint32_t cluster) {
    uint8_t buf[2];
    read_fat(buf, cluster);
    if (geom.fat_type == FAT_TYPE_16) {
        return (uint16_t) ((buf[1] << 8) | buf[0]);
    } else {
        // recompute FAT12 value
        uint32_t offset = cluster + (cluster / 2u);
        uint32_t sector = geom.first_fat_sector + (offset / PENDISK_BLOCK_SIZE);
        uint32_t insec  = offset % PENDISK_BLOCK_SIZE;
        uint8_t b0 = msc_disk[sector][insec];
        uint8_t b1 = (insec + 1u < PENDISK_BLOCK_SIZE) ? msc_disk[sector][insec + 1u] : msc_disk[sector + 1u][0];
        uint16_t both = (cluster & 1u) ? ((uint16_t) b1 << 4) | (b0 >> 4) : ((uint16_t) b1 << 8) | b0;
        return both;
    }
}

static void write_fat_entry(uint32_t cluster, uint16_t value) {
    if (geom.fat_type == FAT_TYPE_16) {
        uint32_t offset = cluster * 2u;
        uint32_t sector = geom.first_fat_sector + (offset / PENDISK_BLOCK_SIZE);
        uint32_t insec  = offset % PENDISK_BLOCK_SIZE;
        msc_disk[sector][insec]     = (uint8_t) (value & 0xFFu);
        msc_disk[sector][insec + 1u] = (uint8_t) ((value >> 8) & 0xFFu);

        // mirror into second FAT
        uint32_t fat2 = geom.first_fat_sector + geom.sectors_per_fat;
        msc_disk[fat2 + (offset / PENDISK_BLOCK_SIZE)][insec]       = (uint8_t) (value & 0xFFu);
        msc_disk[fat2 + (offset / PENDISK_BLOCK_SIZE)][insec + 1u]  = (uint8_t) ((value >> 8) & 0xFFu);
    } else {
        // FAT12
        uint32_t offset = cluster + (cluster / 2u);
        uint32_t sector = geom.first_fat_sector + (offset / PENDISK_BLOCK_SIZE);
        uint32_t insec  = offset % PENDISK_BLOCK_SIZE;
        uint8_t b0 = msc_disk[sector][insec];
        uint8_t b1 = (insec + 1u < PENDISK_BLOCK_SIZE) ? msc_disk[sector][insec + 1u] : msc_disk[sector + 1u][0];

        uint16_t both = (uint16_t) ((b1 << 8) | b0);
        if (cluster & 1u) {
            both = (uint16_t) (both & 0x000Fu) | (uint16_t) (value << 4);
        } else {
            both = (uint16_t) (both & 0xF000u) | value;
        }

        msc_disk[sector][insec]     = (uint8_t) (both & 0xFFu);
        if (insec + 1u < PENDISK_BLOCK_SIZE) {
            msc_disk[sector][insec + 1u] = (uint8_t) ((both >> 8) & 0xFFu);
        } else {
            msc_disk[sector + 1u][0] = (uint8_t) ((both >> 8) & 0xFFu);
        }

        uint32_t fat2 = geom.first_fat_sector + geom.sectors_per_fat;
        msc_disk[fat2 + (offset / PENDISK_BLOCK_SIZE)][insec]     = (uint8_t) (both & 0xFFu);
        if (insec + 1u < PENDISK_BLOCK_SIZE) {
            msc_disk[fat2 + (offset / PENDISK_BLOCK_SIZE)][insec + 1u] = (uint8_t) ((both >> 8) & 0xFFu);
        } else {
            msc_disk[fat2 + (offset / PENDISK_BLOCK_SIZE) + 1u][0] = (uint8_t) ((both >> 8) & 0xFFu);
        }
    }
}

static uint32_t cluster_to_sector(uint32_t cluster) {
    return geom.first_data_sector + ((cluster - 2u) * geom.sectors_per_cluster);
}

//--------------------------------------------------------------------+
// FORMATTING
//--------------------------------------------------------------------+

static void write_boot_sector(void) {
    uint8_t *b = msc_disk[0];
    disk_clear();
    // Reset first sector before writing BPB
    wipe_sector(0);
    b = msc_disk[0];

    memcpy(b + 0, "\xEB\x3C\x90" "PICOUSB  ", 11); // jmp + OEM name
    b[11] = (uint8_t) (geom.bytes_per_sector & 0xFFu);        // bytes per sector lo
    b[12] = (uint8_t) (geom.bytes_per_sector >> 8);           // bytes per sector hi
    b[13] = geom.sectors_per_cluster;
    b[14] = (uint8_t) (geom.reserved_sectors & 0xFFu);        // reserved sectors
    b[15] = (uint8_t) (geom.reserved_sectors >> 8);
    b[16] = geom.num_fats;
    b[17] = (uint8_t) (geom.root_entries & 0xFFu);            // root entries
    b[18] = (uint8_t) (geom.root_entries >> 8);
    b[19] = (uint8_t) (geom.total_sectors & 0xFFu);           // 16-bit total sectors
    b[20] = (uint8_t) ((geom.total_sectors >> 8) & 0xFFu);
    b[21] = 0xF8;                                             // media descriptor
    b[22] = (uint8_t) (geom.sectors_per_fat & 0xFFu);         // sectors per FAT (16-bit)
    b[23] = (uint8_t) (geom.sectors_per_fat >> 8);
    b[24] = 1; b[25] = 0;                                     // sectors per track
    b[26] = 1; b[27] = 0;                                     // number of heads
    b[28] = 0; b[29] = 0; b[30] = 0; b[31] = 0;               // hidden sectors
    if (geom.total_sectors < 65536u) {
        b[32] = 0; b[33] = 0;                                 // large total sectors (only if 16-bit==0)
    } else {
        b[32] = (uint8_t) (geom.total_sectors & 0xFFu);
        b[33] = (uint8_t) ((geom.total_sectors >> 8) & 0xFFu);
        b[34] = (uint8_t) ((geom.total_sectors >> 16) & 0xFFu);
        b[35] = (uint8_t) ((geom.total_sectors >> 24) & 0xFFu);
        b[19] = 0; b[20] = 0;                                 // must be 0 when using 32-bit
    }
    b[36] = 0x80;                                             // drive number
    b[37] = 0x00;
    b[38] = 0x29;                                             // extended boot signature
    b[39] = 0x12; b[40] = 0x34; b[41] = 0x56; b[42] = 0x78;   // volume serial
    memcpy(b + 43, "PICO PENDV", 11);                         // volume label
    if (geom.fat_type == FAT_TYPE_16) {
        memcpy(b + 54, "FAT16   ", 8);
    } else {
        memcpy(b + 54, "FAT12   ", 8);
    }
    b[510] = 0x55;
    b[511] = 0xAA;
}

static void write_fats(void) {
    // Entry 0: media descriptor (0xF8) + pad
    // Entry 1: end-of-cluster marker
    if (geom.fat_type == FAT_TYPE_16) {
        write_fat_entry(0, 0xFFF8);
        write_fat_entry(1, 0xFFFF);
    } else {
        write_fat_entry(0, 0xFF8);
        write_fat_entry(1, 0xFFF);
    }
}

static void mkdir_root_label(void) {
    // First root directory entry: volume label
    uint8_t *e = msc_disk[geom.first_root_sector];
    memset(e, 0, 32);
    memcpy(e, "PICO PENDV", 11);
    e[11] = 0x08; // attribute: volume label
}

// Add one file to the root directory. Returns the first cluster or 0.
static uint32_t add_root_file(const char *name, const char *ext,
                              const uint8_t *content, uint32_t size, uint8_t attr) {
    uint32_t root_start = geom.first_root_sector;

    // Find a free root slot within the root directory region.
    uint32_t slot = 0;
    bool found = false;
    for (uint32_t s = 0; s < geom.root_dir_sectors && !found; s++) {
        uint8_t *rows = msc_disk[root_start + s];
        for (uint32_t r = 0; r < (PENDISK_BLOCK_SIZE / 32u); r++) {
            uint8_t *e = rows + (r * 32u);
            if (e[0] == 0x00) { // empty entry
                found = true;
                break;
            }
            slot++;
        }
        if (found) break;
    }
    if (!found) return 0;

    uint32_t dir_sec  = slot / (PENDISK_BLOCK_SIZE / 32u);
    uint32_t dir_row  = slot % (PENDISK_BLOCK_SIZE / 32u);
    uint8_t  *e       = msc_disk[root_start + dir_sec] + (dir_row * 32u);

    memset(e, 0, 32);
    memcpy(e, name, 8);          // 8.3 name (left padded by caller)
    memcpy(e + 8, ext, 3);       // extension
    e[11] = attr;

    // Date/time (fixed, near epoch)
    e[22] = 0x21; e[23] = 0x48;  // time
    e[24] = 0x60; e[25] = 0x4C;  // date
    e[26] = 0x2F;                // starting cluster hi (0 for FAT12)
    e[27] = 0x00;

    // Find a free data cluster.
    uint32_t cluster = 0;
    uint16_t max_entry = (uint16_t) (geom.cluster_count + 2u);
    for (uint16_t c = 2; c < max_entry; c++) {
        if (get_fat_entry(c) == 0x0000u) {
            cluster = c;
            break;
        }
    }
    if (cluster == 0) {
        if (size == 0) {
            // zero-length file: no cluster needed
            e[14] = (uint8_t) (size & 0xFFu);
            e[15] = (uint8_t) ((size >> 8) & 0xFFu);
            e[28] = (uint8_t) (size & 0xFFu);
            e[29] = (uint8_t) ((size >> 8) & 0xFFu);
            e[30] = (uint8_t) ((size >> 16) & 0xFFu);
            e[31] = (uint8_t) ((size >> 24) & 0xFFu);
            return 0;
        }
        return 0;
    }

    e[26] = (uint8_t) (cluster >> 8);
    e[27] = (uint8_t) (cluster & 0xFFu);
    e[20]              = (uint8_t) (cluster & 0xFFu); // high cluster words (16-bit)
    e[21]              = (uint8_t) (cluster >> 8);
    e[28] = (uint8_t) (size & 0xFFu);
    e[29] = (uint8_t) ((size >> 8) & 0xFFu);
    e[30] = (uint8_t) ((size >> 16) & 0xFFu);
    e[31] = (uint8_t) ((size >> 24) & 0xFFu);

    // Copy the content into the cluster(s) and chain them.
    uint32_t remaining = size;
    uint32_t cur = cluster;
    while (remaining > 0) {
        uint32_t count = geom.sectors_per_cluster * PENDISK_BLOCK_SIZE;
        if (remaining < count) count = remaining;

        uint32_t sec = cluster_to_sector(cur);
        uint32_t written = 0;
        while (written < count) {
            uint32_t chunk = count - written;
            if (chunk > PENDISK_BLOCK_SIZE) chunk = PENDISK_BLOCK_SIZE;
            memcpy(msc_disk[sec + (written / PENDISK_BLOCK_SIZE)] + (written % PENDISK_BLOCK_SIZE),
                   content + (size - remaining) + written, chunk);
            written += chunk;
        }

        remaining -= count;

        // decide next cluster
        if (remaining > 0) {
            uint16_t next = 0;
            for (uint16_t c = 2; c < max_entry; c++) {
                if (get_fat_entry(c) == 0x0000u && c != cur) {
                    next = c;
                    break;
                }
            }
            write_fat_entry(cur, next);
            cur = next;
        } else {
            // mark end-of-chain
            if (geom.fat_type == FAT_TYPE_16) {
                write_fat_entry(cur, 0xFFFF);
            } else {
                write_fat_entry(cur, 0xFFF);
            }
        }
    }

    return cluster;
}

void pendisk_format(void) {
    compute_geometry();

    disk_clear();
    write_boot_sector();
    write_fats();
    mkdir_root_label();

    // Create a default README file on the fresh disk.
    static const char readme[] =
        "Pico Pendrive - Raspberry Pi Pico RP2040\r\n"
        "\r\n"
        "Este dispositivo es un 'pendrive' USB construido con un RP2040.\r\n"
        "El medio es una RAM disko configurable: PENDISK_BLOCK_COUNT bloques\r\n"
        "de 512 bytes (ver msc_disk.h).\r\n"
        "\r\n"
        "Es de lectura/escritura completa. La informacion se pierde al apagar\r\n"
        "porque vive en RAM volatil (un ramdisk), igual que un pendrive real\r\n"
        "pero sin memoria no volatil.\r\n"
        "\r\n"
        "= Pendrive  = Pico\r\n"
        "\r\n"
        "(c) 2026 - MIT License\r\n";

    uint32_t size = (uint32_t) strlen(readme);
    add_root_file("README   ", "TXT", (const uint8_t *) readme, size, 0x20);
}

//--------------------------------------------------------------------+
// MASS STORAGE CALLBACKS (invoked by TinyUSB)
//--------------------------------------------------------------------+

// Invoked when received SCSI_CMD_INQUIRY
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    (void) lun;
    const char vid[8]  = {'P','i','c','o',' ',' ',' ',' '};
    const char pid[16] = {'P','e','n','d','r','i','v','e',' ','D','i','s','k',' ',' ',' '};
    const char rev[4]  = {'1','.','0',' '};
    memcpy(vendor_id,   vid, 8);
    memcpy(product_id,  pid, 16);
    memcpy(product_rev, rev, 4);
}

// Invoked when received Test Unit Ready command
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void) lun;
    if (ejected) {
        // Additional Sense 3A-00 is NOT_FOUND (media removed)
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
        return false;
    }
    return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
    (void) lun;
    *block_count = PENDISK_BLOCK_COUNT;
    *block_size  = PENDISK_BLOCK_SIZE;
}

// Invoked when received Start Stop Unit command
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void) lun;
    (void) power_condition;
    if (load_eject) {
        if (start) {
            // load disk storage
            ejected = false;
        } else {
            // unload disk storage (safe eject)
            ejected = true;
        }
    }
    return true;
}

// Invoked when received READ10 command
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    (void) lun;
    if (lba >= PENDISK_BLOCK_COUNT) return -1;
    const uint8_t* addr = msc_disk[lba] + offset;
    memcpy(buffer, addr, bufsize);
    return (int32_t) bufsize;
}

// Disk is always writable (it is a real RAM pendrive).
bool tud_msc_is_writable_cb(uint8_t lun) {
    (void) lun;
    return !ejected;
}

// Invoked when received WRITE10 command
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    (void) lun;
    if (lba >= PENDISK_BLOCK_COUNT) return -1;
    if (ejected) {
        // Reject writes after a safe eject.
        return -1;
    }
    uint8_t* addr = msc_disk[lba] + offset;
    memcpy(addr, buffer, bufsize);
    return (int32_t) bufsize;
}

// Invoked when received an SCSI command not in built-in list
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
    (void) lun;
    (void) scsi_cmd;
    (void) buffer;
    (void) bufsize;
    // Unknown / unsupported command: report Illegal Request.
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
    return -1;
}
