# APRENDIZAJE — Proyecto micro_sd

Documentación técnica de todo lo aprendido/implementado durante el desarrollo
del firmware `micro_sd` (rama `microsd_card` → `micro_sd_tree`).

Fecha: 2026-09-25 · Firmware v1.5.x / v1.6.0 · Pico W (RP2040) · SDK 2.2.0

---

## 1. Qué hace el firmware

Una Raspberry Pi Pico que:

1. Inicializa una **microSD en modo SPI** (protocolo SD estándar, sin SDIO).
2. Monta el **filesystem FAT** (FatFS R0.15a) de la tarjeta.
3. Muestra en un **OLED SSD1306** un listado tipo `ls`/`tree` de lo que hay
   en la raíz: carpetas primero, archivos después, con tamaños; se desliza
   hacia abajo y al llegar al último vuelve al primero (cíclico).
4. Expone la tarjeta como **pendrive USB** (TinyUSB MSC): la PC monta el
   mismo FAT y puede leer/escribir.
5. LED de estado: parpadea sin host USB, fijo cuando la PC monta el disco.

---

## 2. Hardware y pines

| Dispositivo | Bus  | Pines                                   |
|-------------|------|-----------------------------------------|
| microSD     | SPI1 | SCK=GP10, MOSI=GP11, MISO=GP12, CS=GP13 |
| OLED 128x64 | I2C0 | SDA=GP4, SCL=GP5 (SSD1306 @ 0x3C)       |
| USB         | —    | MSC device (VID 0xCAFE, PID 0x4004)     |
| UART debug  | UART0| TX=GP0 (stdio a 115200)                 |

**Lección clave:** los pines del OLED fueron cambiados de GP2/GP3 (I2C1) a
GP4/GP5. GP2/3 solo funcionan como I2C1; **GP4/5 solo como I2C0**. Un OLED
conectado al bloque I2C equivocado queda en negro sin error de compilación.

Alimentación del módulo microSD: 3.3 V (no 5 V).

---

## 3. Arquitectura del firmware

```
main.cpp            init SD+FAT+USB, banner/tree, loop cooperativo
sd_spi.c            driver microSD SPI (protocolo SD completo)
sd_diskio.c         diskio de FatFS sobre sd_spi.c
file_browser.c      escaneo "/" + orden tipo ls (carpetas primero)
tree_view.c         (rama micro_sd_tree) recorrido recursivo
usb_msc.c           callbacks TinyUSB MSC + descriptores USB
ssd1306.c           driver OLED I2C (framebuffer, font 8x8)
lib/fatfs/          FatFS R0.15a vendido (LFN habilitado)
```

**Concurrencia:** no hay RTOS ni hilos. Loop cooperativo: `tud_task()` (USB)
y el escaneo FAT se alternan; `g_usb_state` evita escanear durante tráfico
USB. FatFS no es thread-safe aquí (`FF_FS_REENTRANT=0`), y con un solo hilo
no hace falta.

**Un solo medio, dos clientes:** FatFS (para el OLED) y TinyUSB MSC (para la
PC) acceden a los mismos sectores de la SD por el mismo `sd_spi.c`. Como los
accesos nunca se solapan (loop único), no hay corrupción.

---

## 4. Driver microSD en modo SPI (sd_spi.c)

Secuencia de init (Physical Layer Simplified Spec):

```
74+ clocks con CS alto y MOSI alto  (la tarjeta arranca en modo SD nativo)
CMD0 (GO_IDLE)          -> R1 = 0x01      (CRC 0x95 válido solo aquí)
CMD8 (0x1AA)            -> R7 con eco     (si responde: SD v2)
ACMD41 (HCS=1 si v2)    -> hasta 0x00     (timeout ~1 s; MMC usa CMD1)
CMD58 (OCR)             -> bit CCS: 1 = SDHC/SDXC (bloces de 512 B)
CMD59 (CRC off), CMD16 (blocklen 512)
SPI 400 kHz -> 10 MHz
```

Detalles que hicieron fallar la primera versión y su fix:

- **CMD0 necesita CS por intento**: algunos clones no entran en idle si CS
  queda alto al terminar el comando → `sd_command()` + `sd_end_command()`
  por cada reintento.
- **Pull-up en MISO**: con CS alto la tarjeta libera el bus; sin pull-up,
  `sd_wait_ready()` y la detección de "no hay tarjeta" leen basura.
- **Dirección de acceso**: SDHC usa LBA directo; SDv1/v2/MMC usan bytes
  (`lba * 512`). Falla silenciosa si se mezclan.
- **Capacidad desde CSD**: en CSD v2, `sectores = (C_SIZE+1) * 1024`
  (512 KiB por unidad, no 1 MB — error clásico que duplica el tamaño).
- **Lectura**: CMD17 → token `0xFE` → 512 bytes → 2 CRC. **Escritura**:
  CMD24 → token `0xFE` → datos → CRC → respuesta `x5` aceptada → esperar
  busy (0x00) hasta 4 s.

`sd_sector_count_cached()` cachea el CSD: el callback USB
`tud_msc_capacity_cb` se llama constantemente y leer el CSD cada vez
ralentizaría la enumeración.

---

## 5. FatFS (lib/fatfs)

- Versión vendida: **R0.15a** (Elm-chan). Solo el núcleo (`ff.c`,
  `ffsystem.c`, `ffunicode.c`); el `diskio` propio vive en `src/sd_diskio.c`.
- `ffconf.h` relevante: `FF_USE_LFN=1` (nombres largos, buffer estático),
  `FF_CODE_PAGE=437`, `FF_MIN_SS=FF_MAX_SS=512`, `FF_USE_MKFS=1`,
  `FF_FS_NORTC=1` (fecha fija 1/1/2022 via `get_fattime()`).
- `SD_AUTO_FORMAT=0` por defecto: **nunca** formatea (no destruye datos);
  una tarjeta sin FAT solo muestra `NO SD`.
- `file_browser.c`: `f_mount(f_mount(&fs,"",1))` → `f_opendir("/")` →
  `f_readdir` → ordenar con `qsort` (carpetas primero, `strcasecmp`).

---

## 6. USB Mass Storage (usb_msc.c)

TinyUSB 0.18 (el que trae SDK 2.2.0) con API nueva:

```c
tusb_rhport_init_t dev_init = { .role = TUSB_ROLE_DEVICE,
                                .speed = TUSB_SPEED_AUTO };
tusb_init(BOARD_TUD_RHPORT, &dev_init);   // tusb_init(void) está DEPRECATED
```

Callbacks MSC implementados (todos requieren exactamente esta firma):

| Callback                  | Función                                    |
|---------------------------|--------------------------------------------|
| `tud_msc_inquiry_cb`      | strings VID/PID que muestra la PC          |
| `tud_msc_test_unit_ready_cb` | siempre true (no hay "eyectar")         |
| `tud_msc_capacity_cb`     | sectores y tamaño (del CSD cacheado)       |
| `tud_msc_read10_cb`       | SCSI READ → `sd_spi_read_sectors()`        |
| `tud_msc_write10_cb`      | SCSI WRITE → `sd_spi_write_sectors()`      |
| `tud_msc_scsi_cb`         | comandos no soportados → sense error       |
| `tud_msc_synchronize_cache_cb` | true (escrituras ya son síncronas)    |

Descriptores: `TUD_CONFIG_DESCRIPTOR` + `TUD_MSC_DESCRIPTOR` (solo MSC,
EP0=64, 100 mA). `CFG_TUD_MSC_EP_BUFSIZE=512` garantiza pedidos alineados
a sector — los callbacks rechazan sub-sectores (`nsectors = bufsize/512`).

**Integración con SDK:** los fuentes de TinyUSB se compilan *dentro* del
target (`tinyusb_device` es INTERFACE con fuentes); el target necesita
`target_include_directories(... lib/tinyusb/src)` para encontrar
`tusb_config.h` (nuestro, en `src/`) y los headers del stack.

---

## 7. OLED SSD1306 (ssd1306.c)

- I2C a 400 kHz, dirección 0x3C, framebuffer 1 KB en RAM (128x64/8).
- Modo de direccionamiento **horizontal**, `0x80` como control byte de
  comando (idéntico al driver oficial de pico-examples).
- Fuente 8x8 **solo mayúsculas**: los nombres de archivo se convierten a
  mayúsculas con `toupper()` (sin `#include <cctype>` no compila en C++).
- 128 px / 8 px = **16 caracteres por fila**; 64/8 = 8 filas (2 de cabecera
  + 6 de archivos). Toda la UI se diseñó dentro de ese límite.

---

## 8. Vista tree (rama micro_sd_tree)

`tree_view.c` recorre el FAT en profundidad y lo aplana a las 6 filas
visibles del OLED, con prefijos de rama ASCII:

```
MICROSD           SDHC   N=12
--------------------------------
/CARPETA1
  +-FOTOS/
  | +-IMG_001.JPG      2M
  +-DOCS/
README.TXT            512
```

- Límite de profundidad (2 niveles) y de nodos totales (FB_MAX_FILES) para
  no desbordar RAM (el struct completo cabe en ~4 KB).
- El re-escaneo periódico reconstruye el árbol; el scroll sigue cíclico.

---

## 9. Build y flasheo

### Estructura de build

- `micro_sd/Makefile` es un **wrapper** que delega en el make generado por
  CMake. Lección: el primer wrapper dependía del ELF como target y **no
  recompilaba** tras cambiar fuentes (make veía el ELF más nuevo que sus
  "dependencias"). Fix: delegar SIEMPRE en el make de CMake (que rastrea
  los .c/.cpp) y verificar que el ELF exista al final.
- El SDK se autodetecta: `$PICO_SDK_PATH`, `../pico-sdk` o `../../pico-sdk`
  (layout `/ruta/pico/pico_src` + `/ruta/pico/pico-sdk`).
- Binarios en `build/src/micro_sd.{elf,uf2}` porque el CMake hace
  `add_subdirectory(src)`.

### Flasheo remoto (la Pico vive en la Raspberry Pi)

```bash
# todo el flujo en la Pi:
ssh joy@raspberry.local "cd /home/joy/src/pico/pico_src && git pull && \
    BOARD=pico_w ./scripts/flash_nosudo_multi.sh micro_sd/"
```

- **No usar BOOTSEL ni picotool desde SSH**: sin reglas udev, picotool no
  puede abrir el USB de la Pico corriendo firmware (`unable to connect`).
  La vía que funciona es **SWD con el Debugprobe** (CMSIS-DAP, `2e8a:000c`)
  y OpenOCD: `program ...elf verify reset exit`.
- `debugprobe-openocd.cfg`: `adapter usb vid_pid` fue **eliminado en
  OpenOCD 0.12** → error de config. Además el `rp2040.cfg` de la distro
  registra `reset-init { rp2xxx rom_api_call 0 CX }` pero el comando
  `rp2xxx` no existe → `Error: args[i] option value ('CX') is not valid`
  (no fatal). Fix: anular el handler (`-event reset-init {}`).
- **Dos clones en la Pi**: `/home/joy/src/pico_src` (viejo, rama main, sin
  SDK hermano — NO usar) y `/home/joy/src/pico/pico_src` (de trabajo, con
  SDK hermano — usar este).

### Versionado

Regla de la casa: **cada push lleva tag y el tag coincide con `VERSION`**.
Bump patch = fix, minor = feature, major = breaking. Flujo:
editar `VERSION` → commit → `git tag vX.Y.Z` → `git push --tags`.

---

## 10. Errores reales encontrados (y su lección)

| Síntoma | Causa raíz | Fix |
|---|---|---|
| `FB_MAX_FILES was not declared` | header usaba macro sin incluir config.h | incluir `config.h` en `file_browser.h` |
| `cyw43_arch_gpio_put not declared` con BOARD=pico_w | rama CYW43 del LED sin `#include pico/cyw43_arch.h` ni `cyw43_arch_init()` | include condicional + init en `led_init()` |
| `toupper not declared` | falta `<cctype>` en C++ | include |
| `keyboard.elf no encontrado` al flashear micro_sd | `config.sh` forzaba `PROJECT_CMAKE_TARGET=pico_keyboard_bridge` si existía carpeta `keyboard/` | el mapeo especial solo aplica a su carpeta |
| OLED en negro | OLED en GP4/5 pero firmware en i2c1/GP2/3 | mover a `i2c0` GP4/5 |
| Nombres en blanco en OLED | fuente solo mayúsculas | `toupper()` al dibujar |
| `make` no recompilaba | wrapper dependía del ELF como target | delegar en make de CMake |
| `Invalid command argument ... ('CX')` en OpenOCD | evento reset-init del rp2040.cfg de distro usa comando inexistente | anular handler en nuestro cfg |
| picotool `unable to connect` por SSH | permisos USB sin udev | usar SWD/OpenOCD con Debugprobe |

---

## 11. Flujo de trabajo (de la casa)

- **Local** = código completo, modificaciones, push.
- **SSH a la Pi** = solo compilar y flashear.
- Cada push: bump de `VERSION` + tag del mismo número.
- Probar siempre el build **local** antes del push; el build remoto es la
  confirmación final.
- Documentar cada error y su causa en este archivo.

---

## 12. Ideas futuras

- Vista alternada nombre/fecha de modificación (FatFS entrega timestamp).
- Entrar a carpetas con un botón (navegación real, no solo raíz).
- Escrituras desde la PC invalidando la caché del árbol en tiempo real
  (hoy se re-escanea cada 3 s).
- Soporte exFAT (tarjetas > 32 GB formateadas de fábrica) via FF_FS_EXFAT.
- Regla udev en la Pi para flashear también por picotool sin sudo.
