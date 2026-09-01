# SKILL.md - Pico Pendrive (Raspberry Pi Pico)

Referencia técnica completa del proyecto **pendrive**. Diseñado para que un agente de IA o desarrollador pueda comprender, modificar y mantener el proyecto de forma autónoma.

---

## 1. Resumen del Proyecto

Pendrive USB realizado íntegramente con una Raspberry Pi Pico RP2040. Emula un dispositivo de Mass Storage (MSC) con un disco FAT **configurable** y **de lectura/escritura completa**, alojado en la SRAM del chip (un ramdisk). Incluye además una interfaz CDC para diagnóstico serial.

- **Lenguaje:** C (C11), C++17 en CMake
- **SDK:** Pico SDK (CMake 3.13+)
- **Firmware:** USB MSC (Mass Storage) + USB CDC
- **Medio:** RAM configurable (por defecto 224 KB), volátil
- **Formato:** FAT12 / FAT16 (se elige automáticamente según tamaño)
- **Versión actual:** v1.0.0
- **Licencia:** MIT
- **Plataforma:** RP2040 (Dual Core ARM Cortex-M0+, 125 MHz)

---

## 2. Arquitectura del Firmware

```
┌─────────────────────────────────────────────────┐
│                   main.c                         │
│                                                  │
│  ┌──────────────┐  ┌──────────────┐             │
│  │ tud_task()   │  │ led_blink()  │             │
│  │ (USB stack)  │  │  (actividad) │             │
│  └──────┬───────┘  └──────┬───────┘             │
│         │                 │                      │
│  ┌──────▼─────────────────▼──────┐              │
│  │    check_serial_commands()    │  CDC debug    │
│  └──────────────┬─────────────────┘              │
│                 │                                │
└─────────────────┼────────────────────────────────┘
                  │
                  ▼
┌──────────────────────────────────────────────────┐
│                  msc_disk.c                       │
│                                                   │
│   ┌─────────────┐   ┌─────────────────────────┐  │
│   │ FAT geometry│   │  RAM disk msc_disk[ ][ ]│  │
│   │ + formatter │ ─►│  (configurable, FAT)    │  │
│   └─────────────┘   └───────────┬─────────────┘  │
│                                 │                │
│   ┌─────────────────────────────▼─────────────┐  │
│   │  TinyUSB MSC callbacks                    │  │
│   │  (READ10 / WRITE10 / CAPACITY / etc.)     │  │
│   └─────────────────────┬─────────────────────┘  │
└─────────────────────────┼────────────────────────┘
                          │
                          ▼
                    USB Host (PC)
                 (aparece un pendrive)
```

### Componentes

| Módulo | Archivo | Responsabilidad |
|--------|---------|-----------------|
| Aplicación | `main.c` | Init, bucle USB, LED, CDC |
| Disco RAM | `msc_disk.c` | Formatea FAT + atiende MSC |
| Config disco | `msc_disk.h` | Parámetros configurables |
| Descriptores | `usb_descriptors.c` | Descriptores USB (MSC+CDC) |
| TinyUSB | `tusb_config.h` | Habilita clases MSC y CDC |

---

## 3. Mapa de Archivos Detallado

```
pico_src/pendrive/
├── CMakeLists.txt              # Build raíz: define PICO_SDK_PATH, incluye src/
├── pico_sdk_import.cmake       # Importador del SDK (no modificar)
├── install_deps.sh             # Instala dependencias del sistema (sudo)
├── flash_pendrive.sh           # Compila + flashea con picotool
├── .gitignore                  # Ignora build/, *.o, *.uf2, etc.
├── src/
│   ├── CMakeLists.txt          # Build del ejecutable: target "pendrive"
│   ├── main.c                  # ★ Bucle principal y USB
│   ├── msc_disk.c              # ★ Disco RAM + FAT + callbacks MSC
│   ├── msc_disk.h              # ★ Configuración (capacidad, etc.)
│   ├── usb_descriptors.c       # Descriptores USB (device, config, strings)
│   └── tusb_config.h           # Configuración TinyUSB (MSC + CDC)
└── docs/
    ├── README.md               # Índice de documentación
    ├── BUILD.md                # Guía de compilación y flasheo
    ├── HARDWARE.md             # Requisitos de hardware
    ├── MANUAL_USO.md           # Manual de usuario final
    └── skill.md                # Este archivo
```

---

## 4. Análisis de `msc_disk.h` (Configuración)

Constantes configurables del disco:

| Constante | Valor default | Descripción |
|-----------|---------------|-------------|
| `PENDISK_BLOCK_SIZE` | 512 | Tamaño de bloque/sector en bytes |
| `PENDISK_BLOCK_COUNT` | 256 | **Capacidad**: número de bloques |
| `PENDISK_SECTORS_PER_CLUSTER` | 1 | Sectores por clúster FAT |
| `PENDISK_ACTIVITY_LED` | 1 | Habilita LED de actividad |

Capacidad resultante = `PENDISK_BLOCK_COUNT × 512` bytes.

| Bloques | Capacidad | Consideración |
|---------|-----------|---------------|
| 256 | 128 KB | Conservador |
| 384 | 192 KB | Más espacio |
| 448 | 224 KB | Default (recomendado) |
| 480 | 240 KB | Máximo aproximado (~264 KB SRAM) |

> ⚠️ Exceder ~480 bloques suele provocar fallo de enlazado por falta de RAM (RFC: el disco se reserva como `static` en `.bss`).

---

## 5. Análisis de `msc_disk.c` (Código del Disco)

### 5.1 Buffer del Disco

```c
static uint8_t msc_disk[PENDISK_BLOCK_COUNT][PENDISK_BLOCK_SIZE];
```

Es el "pendrive" entero en RAM. Se reserva en BSS (no inicializado) y se rellena al formatear.

### 5.2 Geometría FAT (`fat_geom_t`)

Campos calculados por `compute_geometry()`:

| Campo | Descripción |
|-------|-------------|
| `fat_type` | `FAT_TYPE_12` (12) o `FAT_TYPE_16` (16) |
| `sectors_per_fat` | Sectores ocupados por cada FAT |
| `root_dir_sectors` | Sectores del directorio raíz |
| `first_fat_sector` | Sector donde empieza la FAT |
| `first_root_sector` | Sector donde empieza la raíz |
| `first_data_sector` | Sector de los datos de clústeres |
| `cluster_count` | Clústeres disponibles |

`compute_geometry()`:
1. Fija 4 sectores reservados, 2 FATs, 512 entradas raíz.
2. Estima el tamaño de FAT con la fórmula de Microsoft y **itera** hasta que sea estable.
3. Elige FAT12 si `cluster_count < 4085`, si no FAT16.

### 5.3 Leyendas de FAT (helpers)

| Función | Propósito |
|---------|-----------|
| `get_fat_entry(cluster)` | Lee valor de la cadena FAT (12 o 16 bits) |
| `write_fat_entry(cluster, value)` | Escribe + **espeja en la 2ª FAT** |
| `cluster_to_sector(cluster)` | Convierte clúster → sector físico |

En FAT16 cada entrada son 2 bytes; en FAT12 son 1.5 bytes (los helpers manejan el desfase de la media entera).

### 5.4 Formateo (`pendisk_format()`)

Se llama **una vez** al arrancar:

```
1. compute_geometry()           ← calcula todo
2. disk_clear()                 ← pone el disco a ceros
3. write_boot_sector()          ← BPB FAT12/16 en sector 0
4. write_fats()                 ← entradas 0/1 (media + EOC)
5. mkdir_root_label()           ← etiqueta de volumen "PICO PENDV"
6. add_root_file("README"...)   ← archivo de bienvenida
```

#### Boot sector (FAT)
- Jump + OEM `"PICOUSB  "`
- Bytes/sector, sectores/clúster, sectores reservados, nº de FATs
- Entradas raíz, sectores/FAT, media `0xF8`
- Volumen `"PICO PENDV"` y tipo `"FAT12   "`/`"FAT16   "`
- Firmeza `0x55 0xAA` en 510–511

#### `add_root_file()`
Crea un archivo 8.3 en el directorio raíz:
1. Busca una entrada libre en la raíz (`0x00`).
2. Busca un clúster de datos libre (entrada FAT `0x0000`).
3. Escribe nombre, atributos, tamaño, clúster inicial.
4. Copia el contenido a los clústeres encadenando la FAT hasta marcar `EOC` (`0xFFF`/`0xFFFF`).

### 5.5 Callbacks MSC (invocados por TinyUSB)

| Callback | Comportamiento |
|----------|----------------|
| `tud_msc_inquiry_cb` | VID `Pico`, PID `Pendrive Disk`, rev `1.0` |
| `tud_msc_test_unit_ready_cb` | `false` + sense NOT_READY si eject |
| `tud_msc_capacity_cb` | Devuelve `PENDISK_BLOCK_COUNT` × `PENDISK_BLOCK_SIZE` |
| `tud_msc_start_stop_cb` | Gestiona load/eject (`ejected`) |
| `tud_msc_is_writable_cb` | `true` salvo que esté ejectado |
| `tud_msc_read10_cb` | Copia del buffer RAM al host |
| `tud_msc_write10_cb` | Copia del host al buffer RAM (rechaza si eject) |
| `tud_msc_scsi_cb` | Desconocido → sense ILLEGAL_REQUEST |

---

## 6. Análisis de `main.c` (Bucle Principal)

### 6.1 Flujo de Inicialización

```
1. stdio_init_all()
2. gpio LED (PICO_DEFAULT_LED_PIN) como salida
3. pendisk_format()            ← formatea el disco ANTES de enumerar USB
4. cdc_init() → tusb_init()    ← inicia stack USB
5. sleep_ms(500)
6. Banner + INFO por serial CDC
7. while(1):
     tud_task()
     led_blinking_task()
     check_serial_commands()
```

### 6.2 Blink del LED

| Intervalo | Estado |
|-----------|--------|
| 250 ms | No montado |
| 1000 ms | Montado y funcionando |
| 2500 ms | USB suspendido |

### 6.3 Comandos CDC

Buffer de 64 bytes, terminador `\r`/`\n`:

| Comando | Respuesta |
|---------|-----------|
| `INFO` | Bloqueos, tamaño, `ejected`, `mounted` |

---

## 7. Configuración USB (`tusb_config.h`)

| Constante | Valor | Descripción |
|-----------|-------|-------------|
| `CFG_TUD_CDC` | **1** | ✅ CDC habilitado (serial virtual) |
| `CFG_TUD_MSC` | **1** | ✅ Mass Storage habilitado (el pendrive) |
| `CFG_TUD_HID` | 0 | ❌ |
| `CFG_TUD_MIDI` | 0 | ❌ |
| `CFG_TUD_VENDOR` | 0 | ❌ |
| `CFG_TUD_MSC_EP_BUFSIZE` | 512 | Buffer de transferencia MSC |
| `CFG_TUD_CDC_EP_BUFSIZE` | 64 | Buffer de transferencia CDC |
| `CFG_TUD_ENDPOINT0_SIZE` | 64 | Endpoint 0 |

---

## 8. Descriptores USB (`usb_descriptors.c`)

### 8.1 PID automático

```c
#define USB_PID (0x4000 | _PID_MAP(CDC,0) | _PID_MAP(MSC,1) | _PID_MAP(HID,2) | ...)
```
Con CDC+MSC activos: PID = `0x4001`. VID = `0xCafe`, USB 2.0.

### 8.2 Configuration Descriptor

```
ITF_NUM_CDC       = 0
ITF_NUM_CDC_DATA  = 1
ITF_NUM_MSC       = 2
EPNUM_CDC_NOTIF = 0x81, EPNUM_CDC_OUT = 0x02, EPNUM_CDC_IN = 0x82
EPNUM_MSC_OUT    = 0x03, EPNUM_MSC_IN    = 0x83
```
Usa **IAD** (Interface Association Descriptor) porque combina CDC + MSC.

### 8.3 String Descriptors

| Índice | Cadena |
|--------|--------|
| 1 | `"Pico Pendrive"` (Manufacturer) |
| 2 | `"Pico Pendrive MSC"` (Product) |
| 3 | `"1234567890"` (Serial) |
| 4 | `"Pico CDC"` (Interface CDC) |
| 5 | `"Pico Disk"` (Interface MSC) |

---

## 9. Build System (CMake)

### 9.1 `CMakeLists.txt` (raíz)

```cmake
cmake_minimum_required(VERSION 3.13)
set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../pico-sdk")
include(pico_sdk_import.cmake)
project(pendrive C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_subdirectory(src)
```

### 9.2 `src/CMakeLists.txt`

```cmake
add_executable(pendrive main.c usb_descriptors.c msc_disk.c)
target_include_directories(pendrive PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(pendrive PUBLIC
    pico_stdlib         # Core SDK
    tinyusb_device      # USB device stack
    tinyusb_board       # Board support USB
)
pico_add_extra_outputs(pendrive)  # Genera .uf2, .bin, .elf, .map
```

No se necesita `hardware_flash` ni drivers de periféricos extra; la clase MSC la aporta TinyUSB.

---

## 10. Constantes Clave

```c
// === Disco (msc_disk.h) ===
#define PENDISK_BLOCK_SIZE          512u
#define PENDISK_BLOCK_COUNT         448u   // ← capacidad configurable (224 KB default)
#define PENDISK_SECTORS_PER_CLUSTER 1u

// === Geometría FAT (msc_disk.c) ===
geom.reserved_sectors = 4;
geom.num_fats         = 2;
geom.root_entries     = 512;
genre.media           = 0xF8;

// === USB (tusb_config.h) ===
CFG_TUD_MSC = 1
CFG_TUD_CDC = 1
```

---

## 11. Flujo de Estados USB

```
      ┌────────────────┐
      │   INICIALIZAR  │
      │ pendisk_format │
      │ cdc_init()     │
      └───────┬────────┘
              ▼
      ┌────────────────┐         ┌───────────────┐
      │  ESPERANDO USB │◄───────►│ MOUNTED       │
      │ (LED 250ms)    │ enumerar │ (LED 1000ms) │
      └───────┬────────┘         └───────┬───────┘
              │                           │ READ10/WRITE10
              │                           ▼
              │                    ┌───────────────┐
              └─────────► SUSPENDED│ (LED 2500ms) │
                                  └───────────────┘
```

Safe-eject: el host envía START STOP UNIT → `ejected = true` → las escrituras se rechazan (`tud_msc_is_writable_cb` = `false`, `write10` devuelve -1).

---

## 12. Scripts

### 12.1 `flash_pendrive.sh`

Igual patrón que `flash_adc_oled.sh`, 3 modos:

| Flag | Acción |
|------|--------|
| (sin flag) | Compilar + flashear + verificar |
| `--compile-only` | Solo compilar |
| `--flash-only` | Solo flashear |

Verifica el dispositivo MSC con `lsusb | grep "cafe:4001"` (PID 0x4001).

Output: `build/src/pendrive.uf2`.

### 12.2 `install_deps.sh`

Instala build-essential, cmake, arm-none-eabi, python3, libusb. Verifica picotool.

---

## 13. Troubleshooting

| Problema | Causa probable | Solución |
|----------|----------------|----------|
| Pendrive no aparece | Firmware no flasheado | Repetir BOOTSEL + copiar `.uf2` |
| Sale menos espacio | FAT/raíz consumen algo | Es normal; revisa `PENDISK_BLOCK_COUNT` |
| Se pierden archivos | RAM volátil (ramdisk) | Normal; no persiste al apagar |
| Linker "out of memory" | Demasiados bloques | Bajar `PENDISK_BLOCK_COUNT` (< ~480) |
| Build falla: PICO_SDK_PATH | SDK no encontrado | Verificar `../../pico-sdk` |
| El disco está "solo lectura" | Eject de seguridad | Desconectar/reconectar USB |
| `picotool` no encontrado | No compilado | Compilar desde `/mnt/disk/src/rpico/picotool` |

---

## 14. Dependencias del Build

| Dependencia | Ubicación | Uso |
|-------------|-----------|-----|
| Pico SDK | `../../pico-sdk` | Core del SDK |
| TinyUSB | `pico-sdk/lib/tinyusb` | Stack USB (MSC + CDC) |
| pico_stdlib | Pico SDK | Funciones básicas |
| tinyusb_device | Pico SDK | USB device |
| tinyusb_board | Pico SDK | Board support |

---

## 15. Extensibilidad

### Cambiar la capacidad del pendrive

```c
// msc_disk.h
#define PENDISK_BLOCK_COUNT   384u   // 192 KB
```

### Hacer el disco mayoritariamente persistente

- Usar los últimos sectores de la **flash QSPI** del Pico y mapear READ10/WRITE10 a `flash_range_program()` / lectura XIP.
- Volátil vs persistente: RAM es rápida y sin límite de escrituras; flash es lenta y limitada.

### Agregar nombres largos (VFAT)

- Implementar entradas LFN (0x0F) concatenadas antes de la entrada 8.3 en el directorio.

### Agregar segundo LUN (dos discos)

- Ampliar `msc_disk[2][...]` y atender `lun` en los callbacks.

### Escanear clústeres libres

- `get_fat_entry(c) == 0x0000` ⇒ libre (usado para ubicar archivos).

---

## 16. Diagrama de Estados de `pendisk_format()`

```
        ┌───────────────┐
        │  START        │
        └──────┬────────┘
               ▼
        ┌───────────────┐      iterate
        │ compute_geom  │──────────────┐
        └──────┬────────┘              │
               ▼                       │
        ┌───────────────┐   no estable │
        │ fat_size fijo?│──────────────┘
        └──────┬────────┘
               │ sí
               ▼
        ┌───────────────┐
        │ disk_clear()  │
        └──────┬────────┘
               ▼
        ┌───────────────┐
        │ boot_sector   │
        └──────┬────────┘
               ▼
        ┌───────────────┐
        │ write_fats()  │
        └──────┬────────┘
               ▼
        ┌───────────────┐
        │ root label    │
        └──────┬────────┘
               ▼
        ┌───────────────┐
        │ add README    │
        └──────┬────────┘
               ▼
             DONE
```
