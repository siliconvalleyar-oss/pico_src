/*
 * config.h - Configuración central del proyecto micro_sd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Mapa de pines (RP2040):
 *   microSD  -> SPI1: SCK=GP10, MOSI=GP11, MISO=GP12, CS=GP13
 *   OLED     -> I2C1: SDA=GP2,  SCL=GP3 (SSD1306 128x64 @ 0x3C)
 *   USB      -> MSC (pendrive): la PC ve el contenido FAT de la microSD
 */

#ifndef MICRO_SD_CONFIG_H
#define MICRO_SD_CONFIG_H

//--------------------------------------------------------------------+
// microSD (modo SPI)
//--------------------------------------------------------------------+
#define SD_SPI_INST          spi1        // GP10/11/12 pertenecen a SPI1
#define SD_PIN_SCK           10
#define SD_PIN_MOSI          11
#define SD_PIN_MISO          12
#define SD_PIN_CS            13          // CS manual (gpio, no función SPI)

#define SD_SPI_INIT_HZ       400000u     // identificación (<= 400 kHz)
#define SD_SPI_FAST_HZ       10000000u   // transferencia (10 MHz)

#define SD_SECTOR_SIZE       512u

// Si la tarjeta no tiene filesystem, formatearla automáticamente.
// 0 = NO formatear (seguro, no destruye datos). La PC ofrecerá formatear.
#define SD_AUTO_FORMAT       0

//--------------------------------------------------------------------+
// OLED SSD1306
//--------------------------------------------------------------------+
#define OLED_I2C_INST        i2c0        // GP4/GP5 pertenecen a I2C0
#define OLED_SDA_PIN         4
#define OLED_SCL_PIN         5

//--------------------------------------------------------------------+
// USB (TinyUSB MSC)
//--------------------------------------------------------------------+
#define USB_VID              0xCAFEu
#define USB_PID              0x4004u
#define USB_MANUFACTURER     "siliconvalleyar"
#define USB_PRODUCT          "PicoSD-MSC"

//--------------------------------------------------------------------+
// Banner de archivos en el OLED
//--------------------------------------------------------------------+
#define BANNER_ROWS          6           // filas de archivos visibles (y16..y56)
#define BANNER_SCROLL_MS     700         // velocidad del deslizamiento
#define FB_RESCAN_MS         3000        // re-escaneo periódico del directorio
#define FB_MAX_FILES         60          // archivos listados como máximo
#define FB_NAME_LEN          32          // largo máximo de nombre mostrado

//--------------------------------------------------------------------+
// LED de estado
//--------------------------------------------------------------------+
#define LED_PIN_FALLBACK     25          // Pico (no W): LED onboard en GP25

#endif // MICRO_SD_CONFIG_H
