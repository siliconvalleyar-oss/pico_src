# micro_sd — Pico + microSD + OLED + USB MSC

Firmware para Raspberry Pi Pico (RP2040) que:

1. **Lee una tarjeta microSD** por SPI (FAT12/16/32 con FatFS).
2. **Muestra en el OLED SSD1306 un banner deslizante** con los archivos encontrados en la raíz: se desliza hacia abajo y, al llegar al último, vuelve a mostrar el primero (scroll cíclico infinito).
3. **Expone la tarjeta como pendrive USB** (TinyUSB Mass Storage): la PC ve todo el FAT de la microSD y puede leer/escribir.

## Hardware

| Dispositivo | Bus  | Pines                          |
|-------------|------|--------------------------------|
| microSD     | SPI1 | SCK=**GP10**, MOSI=**GP11**, MISO=**GP12**, CS=**GP13** |
| OLED 128x64 | I2C1 | SDA=**GP2**, SCL=**GP3** (0x3C) |
| USB         | —    | MSC device (VID 0xCAFE, PID 0x4004) |

Alimentación del módulo microSD: 3.3 V (¡no 5 V!).

## Pantalla

```
+----------------------------+
| MICROSD (label del volumen)|
| SDHC                N=12   |
| -------------------------------- |
| ARCHIVO1.TXT               |
| CARPETA                    |
| FOTO.JPG                   |
| ...  (6 filas visibles)    |
|                            |
+----------------------------+
```

- El banner avanza una fila cada 700 ms (`BANNER_SCROLL_MS`).
- El directorio se re-escanea cada 3 s (`FB_RESCAN_MS`).
- Si no hay tarjeta: muestra `NO SD` y reintenta cada 2 s.
- LED: parpadea con USB desconectado, fijo cuando la PC monta el disco.

## Compilar

Desde la carpeta del proyecto (hay un Makefile wrapper sobre CMake):

```bash
cd micro_sd
make -j4
```

Salidas en `build/src/`: `micro_sd.uf2`, `micro_sd.elf`.

Requiere `PICO_SDK_PATH` (o SDK hermano del repo: `../pico-sdk` o `../../pico-sdk`) con los **submódulos inicializados** (`git submodule update --init` — TinyUSB está en `lib/tinyusb`).

## Flashear

### Remota (Raspberry Pi con la Pico conectada), un solo paso

```bash
./scripts/flash_remote.sh            # push + pull + make + picotool load
./scripts/flash_remote.sh --no-push  # sin push local
```

Equivale a: compilar en la Pi por SSH y flashear con `picotool`:

```bash
ssh joy@raspberry.local "cd /home/joy/src/pico/pico_src && git pull && cd micro_sd && make -j4"
ssh joy@raspberry.local "picotool reboot -u -f; sleep 3; picotool load .../micro_sd.uf2 -f && picotool reboot"
```

### Local (BOOTSEL)

Copiar `build/src/micro_sd.uf2` a la Pico en modo BOOTSEL.

## Debug

`stdio` sale por **UART0** (GP0=TX, GP1=RX) a 115200. USB queda reservado para MSC.

```
=== PICO + microSD + OLED + USB MSC ===
SD: SDHC, 62333184 sectores
FAT: MICROSD - 12 archivos, 14124/29896 MB libres
USB MSC: listo
```

## Arquitectura

```
main.cpp            init SD+FAT+USB, banner deslizante, loop cooperativo
sd_spi.c            driver microSD SPI (CMD0/8/41/58/16, CMD17/24)
sd_diskio.c         diskio de FatFS sobre sd_spi.c
file_browser.c      escaneo de "/" (FatFS) para el banner
usb_msc.c           callbacks TinyUSB MSC + descriptores USB
ssd1306.c           driver OLED I2C (framebuffer, font 8x8)
lib/fatfs/          FatFS R0.15a (Elm-chan), LFN habilitado
```

El acceso al medio es exclusivo por turno: el loop cooperativo atiende USB
(`tud_task`) o navega el FAT, nunca ambos a la vez. `usb_msc.c` exporta
`g_usb_state` para que el banner evite escanear durante tráfico USB.

## Notas

- La PC monta la tarjeta directamente: no se necesita formatearla desde el
  firmware. `SD_AUTO_FORMAT=0` (default) **nunca** destruye datos.
- Escrituras desde la PC: recomendable "expulsar de forma segura" antes de
  desconectar (las escrituras SPI son síncronas, pero el SO puede cachear).
- Tarjetas soportadas: MMC, SD v1, SD v2 y SDHC/SDXC (hasta ~2 TB teóricos).

## Licencia

BSD-3-Clause para el código del proyecto. FatFS: licencia propia de Elm-chan (BSD-style).
