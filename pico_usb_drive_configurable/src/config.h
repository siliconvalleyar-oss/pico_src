/*
 * config.h - All compile-time configuration for the Configurable USB Pendrive
 *
 * SPDX-License-Identifier: MIT
 *
 * This header centralises EVERY setting that can be tuned before compiling.
 * Each parameter documents its allowed range and what happens if you set an
 * out-of-range value. Nothing in this file is expected to be edited by an
 * end user; runtime settings live in the config.txt file on the pendrive.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

//--------------------------------------------------------------------+
// FLASH LAYOUT (W25Q16 - 2 MB onboard flash)
//--------------------------------------------------------------------+
/*
 * OFFSET_EN_FLASH
 *   Address (byte) where the FAT storage area starts inside the 2 MB flash.
 *   The firmware (our .uf2) lives below this offset. Must be sector aligned
 *   (multiple of 4096, the flash sector size) and 4-KB aligned.
 *
 *   Allowed range : 0x10000 .. (0x200000 - TAMAÑO_MAXIMO_EN_BYTES)
 *   Default       : 0x80000 (512 KB) -> leaves 512 KB for the firmware image.
 *   Warning       : If too small, the build may overflow into the data area.
 */
#define OFFSET_EN_FLASH           0x80000u

/*
 * TAMAÑO_MAXIMO_EN_BYTES
 *   Size of the storage window the pendrive exposes. The whole region
 *   [OFFSET_EN_FLASH, OFFSET_EN_FLASH + TAMAÑO_MAXIMO_EN_BYTES) is the disk.
 *
 *   Allowed range : 4096 .. (0x200000 - OFFSET_EN_FLASH). Must be 4096 aligned.
 *   Default       : 0x180000 (1.5 MB). With OFFSET 0x80000 this fits the 2 MB
 *                   flash exactly (0x80000 + 0x180000 = 0x200000).
 *   Note          : The FS is FAT16; keep size <= 2 GB in sectors of 512.
 */
#define TAMAÑO_MAXIMO_EN_BYTES    0x180000u   // 1,572,864 bytes = 1.5 MB

// Number of 512-byte logical sectors (derived). Kept as a convenience macro.
#define DISK_SECTOR_SIZE          512u
#define DISK_SECTOR_COUNT         (TAMAÑO_MAXIMO_EN_BYTES / DISK_SECTOR_SIZE)

//--------------------------------------------------------------------+
// GPIO / I2C / LED
//--------------------------------------------------------------------+
/*
 * GPIO_LED_ESTADO
 *   GPIO pin of the status LED. The RP2040 has an onboard LED on GPIO 25.
 *   Any output-capable GPIO (0..28) is allowed.
 *
 *   Default: 25 (onboard LED). If you wire an external LED, set its GPIO here.
 */
#define GPIO_LED_ESTADO           25

/*
 * I2C_SDA_PIN / I2C_SCL_PIN
 *   GPIO pins used for the SSD1306 OLED (I2C). Any GPIO with pull-up works,
 *   but hardware-I2C requires valid SDA/SCL pairs for the chosen I2C block.
 *   Defaults 4 and 5 are I2C0 on a standard Pico.
 *
 *   Allowed range : 0..28. SCL must differ from SDA.
 */
#define I2C_SDA_PIN               16
#define I2C_SCL_PIN               17

/*
 * OLED_ADDR
 *   7-bit I2C address of the SSD1306. Common values: 0x3C or 0x3D.
 *
 *   Allowed range : 0x08..0x77 (valid 7-bit I2C addresses).
 *   Default       : 0x3C (most common for 128x64 I2C OLED modules).
 */
#define OLED_ADDR                 0x3Cu

/*
 * OLED_WIDTH / OLED_HEIGHT
 *   Display dimensions in pixels. Supported SSD1306 panels are 128x64 and
 *   128x32. The driver is written generically; adjust if you use 128x32.
 *
 *   Default: 128x64.
 */
#define OLED_WIDTH                128u
#define OLED_HEIGHT               64u

//--------------------------------------------------------------------+
// USB IDENTIFICATION
//--------------------------------------------------------------------+
/*
 * USB_VID / USB_PID
 *   USB Vendor and Product identifiers. These appear in `lsusb`.
 *
 *   Allowed range : 0x0000..0xFFFF (16-bit USB IDs).
 *   Defaults      : 0xCAFE / 0x4000 (Pico SDK convention for custom devices).
 *   Warning       : For a real product use a vendor-assigned VID.
 */
#define USB_VID                   0xCAFEu
#define USB_PID                   0x4000u

/*
 * USB_MANUFACTURER / USB_PRODUCT
 *   USB string descriptors. Shown by the OS in device details / lsusb.
 *   Keep them printable ASCII; length is arbitrary (TinyUSB binds them).
 *
 *   Defaults: "MyCompany" / "PicoDrive".
 */
#define USB_MANUFACTURER          "MyCompany"
#define USB_PRODUCT               "PicoDrive"

//--------------------------------------------------------------------+
// FILESYSTEM
//--------------------------------------------------------------------+
/*
 * FAT_SECTOR_SIZE
 *   Logical sector size of the FAT volume. The flashcards use 512 B.
 *
 *   Allowed range: 512 (recommended, universally compatible). Some stacks
 *   allow 4096, but 512 is safest for FAT16 + TinyUSB MSC.
 */
#define FAT_SECTOR_SIZE           512u

//--------------------------------------------------------------------+
// RUNTIME DEFAULTS
//--------------------------------------------------------------------+
/*
 * The following are the FALLBACK values used when config.txt is missing or
 * the pendrive is first booted (they seed the auto-created config.txt).
 * They match the keys documented in docs/CONFIGURATION_GUIDE.md.
 */
#define CFG_DEFAULT_VOLUME_LABEL  "MiPendrive"
#define CFG_DEFAULT_READ_ONLY     0
#define CFG_DEFAULT_ENABLE_OLED   1
#define CFG_DEFAULT_LED_ON_CONNECT 1
#define CFG_DEFAULT_AUTO_MOUNT_DELAY_MS 500u

//--------------------------------------------------------------------+
// CONFIG WATCHER
//--------------------------------------------------------------------+
/*
 * CONFIG_POLL_INTERVAL_MS
 *   How often (ms) the firmware re-reads config.txt to detect hot-plug
 *   changes (timeout/hash comparison). Default: 2000 ms (per spec).
 */
#define CONFIG_POLL_INTERVAL_MS   2000u

/*
 * USB Mount is delayed at boot by CFG_DEFAULT_AUTO_MOUNT_DELAY_MS so the
 * OS sees the device only after the filesystem is formatted/ready.
 */

#endif /* _CONFIG_H_ */
