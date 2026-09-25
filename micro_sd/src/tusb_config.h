/*
 * tusb_config.h - Configuración de TinyUSB para micro_sd
 *
 * SPDX-License-Identifier: MIT
 *
 * Solo Mass Storage (MSC) device: la PC ve la microSD como pendrive.
 */

#ifndef MICRO_SD_TUSB_CONFIG_H
#define MICRO_SD_TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//------------- Board specific -------------//
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif

#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

//------------- Common -------------//
// CFG_TUSB_MCU y CFG_TUSB_OS los define el SDK (OPT_MCU_RP2040 / OPT_OS_PICO)
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG        0
#endif

#define CFG_TUD_ENABLED       1
#define CFG_TUD_MAX_SPEED     BOARD_TUD_MAX_SPEED

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN    __attribute__ ((aligned(4)))
#endif

//------------- Device -------------//
#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE    64
#endif

//------------- Class -------------//
#define CFG_TUD_CDC               0
#define CFG_TUD_MSC               1
#define CFG_TUD_HID               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0

// Buffer de MSC (un sector completo)
#define CFG_TUD_MSC_EP_BUFSIZE    512

#ifdef __cplusplus
}
#endif

#endif // MICRO_SD_TUSB_CONFIG_H
