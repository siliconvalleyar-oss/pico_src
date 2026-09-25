/*
 * usb_msc.h - TinyUSB MSC: la PC ve la microSD como pendrive
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MICRO_SD_USB_MSC_H
#define MICRO_SD_USB_MSC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Estado del enlace USB (para el OLED). */
typedef struct {
    bool mounted;   // host configurado el dispositivo
    bool reading;   // READ10 en curso
    bool writing;   // WRITE10 en curso
} usb_state_t;

extern usb_state_t g_usb_state;

/* Inicializa TinyUSB device (MSC). Llamar despues de montar la SD. */
void usb_msc_init(void);

/* Bombea eventos USB; llamar constantemente en el loop principal. */
void usb_msc_task(void);

#ifdef __cplusplus
}
#endif

#endif // MICRO_SD_USB_MSC_H
