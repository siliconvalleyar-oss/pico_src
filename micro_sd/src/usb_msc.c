/*
 * usb_msc.c - TinyUSB MSC device: expone la microSD como pendrive USB
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Los callbacks tud_msc_* traducen SCSI READ10/WRITE10 a accesos por sector
 * en la microSD (mismo medio que FatFS usa para el OLED). Con el loop
 * cooperativo de main.cpp el acceso es exclusivo por turno: o atiende USB
 * (tud_task) o el firmware navega el FAT; nunca simultaneamente.
 *
 * Identificacion USB:
 *   VID 0xCAFE PID 0x4004  "siliconvalleyar PicoSD-MSC"
 */

#include <string.h>

#include "tusb.h"

#include "config.h"
#include "usb_msc.h"

usb_state_t g_usb_state;

//--------------------------------------------------------------------+
// Descriptores
//--------------------------------------------------------------------+

#define USB_BCD 0x0200

static uint16_t s_string_desc[32];

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,
    .bDeviceClass       = 0x00,   // clase por interfaz (MSC)
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

enum {
    ITF_NUM_MSC = 0,
    ITF_NUM_TOTAL
};
#define EPNUM_MSC_OUT 0x01
#define EPNUM_MSC_IN  0x81

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Config 1, n interfaces, string 0, longitud, atributos, consumo 100 mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 1, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

static char const *s_string_desc_arr[] = {
    [STRID_LANGID]       = "Raspberry Pi",
    [STRID_MANUFACTURER] = USB_MANUFACTURER,
    [STRID_PRODUCT]      = USB_PRODUCT,
    [STRID_SERIAL]       = "000001",
};

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    if (index == 0) {
        s_string_desc[1] = 0x0409;              // English (US)
        s_string_desc[0] = (4 << 8) | 1;        // langid string
        return s_string_desc;
    }
    if (index >= 4) return NULL;

    uint8_t len = (uint8_t) strlen(s_string_desc_arr[index]);
    if (len > 31) len = 31;
    s_string_desc[0] = (uint16_t) (((2 + len) << 8) | 1);
    for (uint8_t i = 0; i < len; i++) {
        s_string_desc[1 + i] = s_string_desc_arr[index][i];
    }
    return s_string_desc;
}

//--------------------------------------------------------------------+
// Callbacks MSC (invocados desde C -> extern "C" implicito en .c)
//--------------------------------------------------------------------+

// SCSI INQUIRY: strings de identidad
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8],
                        uint8_t product_id[16], uint8_t product_rev[4]) {
    (void) lun;
    char v[8], p[16];
    memset(v, ' ', sizeof(v));
    memset(p, ' ', sizeof(p));
    size_t vl = strlen(USB_MANUFACTURER);
    if (vl > sizeof(v)) vl = sizeof(v);
    memcpy(v, USB_MANUFACTURER, vl);
    size_t pl = strlen(USB_PRODUCT);
    if (pl > sizeof(p)) pl = sizeof(p);
    memcpy(p, USB_PRODUCT, pl);
    memcpy(vendor_id, v, 8);
    memcpy(product_id, p, 16);
    product_rev[0] = '1';
    product_rev[1] = '.';
    product_rev[2] = '0';
    product_rev[3] = ' ';
}

// TEST UNIT READY: listo si la SD respondio al montaje
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void) lun;
    return true;
}

// READ CAPACITY (10)
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    (void) lun;
    extern uint32_t sd_sector_count_cached(void);
    *block_count = sd_sector_count_cached();
    *block_size  = SD_SECTOR_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition,
                           bool start, bool load_eject) {
    (void) lun; (void) power_condition; (void) start; (void) load_eject;
    return true;
}

bool tud_msc_is_writable_cb(uint8_t lun) {
    (void) lun;
    return true;
}

// READ10: lba/offset sobre la microSD
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                          void *buffer, uint32_t bufsize) {
    (void) lun;
    extern bool sd_spi_read_sectors(uint32_t lba, uint8_t *buf, uint32_t n);

    // Solo pedidos alineados a sector (CFG_TUD_MSC_EP_BUFSIZE=512 garantiza 512)
    uint32_t nsectors = bufsize / SD_SECTOR_SIZE;
    if (nsectors == 0) return -1;

    g_usb_state.reading = true;
    bool ok = sd_spi_read_sectors(lba, (uint8_t *) buffer, nsectors);
    g_usb_state.reading = false;
    if (!ok) return -1;
    return (int32_t) bufsize;
}

// WRITE10
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           uint8_t *buffer, uint32_t bufsize) {
    (void) lun;
    extern bool sd_spi_write_sectors(uint32_t lba, const uint8_t *buf, uint32_t n);

    uint32_t nsectors = bufsize / SD_SECTOR_SIZE;
    if (nsectors == 0) return -1;

    g_usb_state.writing = true;
    bool ok = sd_spi_write_sectors(lba, buffer, nsectors);
    g_usb_state.writing = false;
    if (!ok) return -1;
    return (int32_t) bufsize;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16],
                        void *buffer, uint16_t bufsize) {
    (void) lun; (void) scsi_cmd; (void) buffer; (void) bufsize;
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
    return -1;
}

// SYNCHRONIZE CACHE: FatFS ya escribe sincrono; nada pendiente
bool tud_msc_synchronize_cache_cb(uint8_t lun) {
    (void) lun;
    return true;
}

//--------------------------------------------------------------------+
// Eventos de dispositivo
//--------------------------------------------------------------------+

void tud_mount_cb(void)   { g_usb_state.mounted = true; }
void tud_umount_cb(void)  { g_usb_state.mounted = false; }
void tud_suspend_cb(bool remote_wakeup_en) { (void) remote_wakeup_en; }
void tud_resume_cb(void)  {}

//--------------------------------------------------------------------+
// Publico
//--------------------------------------------------------------------+

void usb_msc_init(void) {
    memset(&g_usb_state, 0, sizeof(g_usb_state));

    tusb_rhport_init_t dev_init = {
        .role  = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);
}

void usb_msc_task(void) {
    tud_task();
}
