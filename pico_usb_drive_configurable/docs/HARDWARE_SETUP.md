# Hardware Setup

## Bill of materials

| Component | Notes |
|-----------|-------|
| Raspberry Pi Pico (RP2040) | Any version; the 2 MB W25Q16 flash is onboard |
| SSD1306 OLED 128x64 (I2C) | Optional, 4-pin (VCC/GND/SCL/SDA) |
| Status LED | Optional (falls back to the onboard GPIO 25 LED) |
| USB cable / micro-USB | Data-capable cable for flashing and use |

No external storage is required — the pendrive uses the Pico's **onboard 2 MB
SPI flash (W25Q16)**, so the only external hardware is the optional OLED and
LED.

---

## Wiring

### SSD1306 OLED (I2C) — optional

| SSD1306 pin | Pico GPIO (default) |
|-------------|---------------------|
| VCC (3.3 V) | 3V3 (OUT) |
| GND         | GND |
| SCL         | GPIO 17 (I2C0 SCL) |
| SDA         | GPIO 16 (I2C0 SDA) |

Pins are configurable via `I2C_SDA_PIN` / `I2C_SCL_PIN` in `src/config.h`
(they must be a valid hardware-I2C pair on the same I2C block).

> These are 3.3 V logic. If your OLED board has its own regulator, ensure it
> still runs at 3.3 V I2C levels.

### Status LED — optional

| LED | Pico GPIO (default) |
|-----|---------------------|
| Anode (+) via resistor (~220 Ω) | GPIO 25 (onboard LED) |

If you omit the external LED the onboard LED on GPIO 25 is used.

---

## Flash usage note (very important)

The pendrive uses the **same physical flash** that stores the firmware. The
two regions are separated at compile time:

- Firmware: `[0x000000 .. 0x00080000)` (default 512 KB).
- FAT pendrive: `[0x00080000 .. 0x00200000)` (default 1.5 MB).

`flash_range_erase()`/`flash_range_program()` from the pico-sdk are XIP-safe
and disable interrupts briefly during a program/erase. Because the data region
starts at 512 KB and our firmware is well below that, live flash writes never
corrupt the running firmware.

**Never** move `OFFSET_EN_FLASH` below the actual firmware size, or the
firmware would be overwritten by pendrive data.

---

## USB connection

- Use a **data** cable (not charge-only).
- The Pico enumerates as a Mass Storage device. No drivers are needed on
  Linux, Windows or macOS.
- The drive holds ~1.5 MB of usable FAT16 space.
