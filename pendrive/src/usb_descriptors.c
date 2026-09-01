/*
 * USB Descriptors for pendrive project
 * USB Mass Storage (MSC) + USB CDC (Virtual Serial Port)
 *
 * SPDX-License-Identifier: MIT
 */

#include "bsp/board_api.h"
#include "tusb.h"

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

// Auto ProductID layout's Bitmap:
//   [MSB]         HID | MSC | CDC          [LSB]
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

#define USB_VID           0xCafe
#define USB_BCD           0x0200

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,

    // Use Interface Association Descriptor (IAD) for CDC
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_MSC,
    ITF_NUM_TOTAL
};

#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82

#define EPNUM_MSC_OUT     0x03
#define EPNUM_MSC_IN      0x83

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // CDC Interface
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

    // MSC Interface
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_CDC_IF,
    STRID_MSC_IF,
};

static char const *string_desc_arr[] = {
    [STRID_LANGID]        = "Raspberry Pi",      // Supported Language
    [STRID_MANUFACTURER]  = "Pico Pendrive",     // Manufacturer
    [STRID_PRODUCT]       = "Pico Pendrive MSC", // Product
    [STRID_SERIAL]        = "1234567890",        // Serial Number
    [STRID_CDC_IF]        = "Pico CDC",          // CDC Interface
    [STRID_MSC_IF]        = "Pico Disk",         // MSC Interface
};

static uint16_t string_desc[32];

uint16_t const * tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;

    if (index == 0) {
        string_desc[1] = 0x0409; // English (US)
        string_desc[0] = (4 << 8) | 1; // Language ID string length = 4 bytes
        return string_desc;
    }

    if (index >= 6) return NULL;

    // Convert ASCII to UTF-16
    uint8_t len = strlen(string_desc_arr[index]);
    if (len > 31) len = 31;

    string_desc[0] = ((2 + len) << 8) | 1;
    for (uint8_t i = 0; i < len; i++) {
        string_desc[1 + i] = string_desc_arr[index][i];
    }

    return string_desc;
}
