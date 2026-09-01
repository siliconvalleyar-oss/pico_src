# SKILL.md - Osciloscopio Digital KY-037 (Raspberry Pi Pico)

Referencia técnica completa del proyecto **adc_oled**. Diseñado para que un agente de IA o desarrollador pueda comprender, modificar y mantener el proyecto de forma autónoma.

---

## 1. Resumen del Proyecto

Osciloscopio digital basado en Raspberry Pi Pico RP2040 que usa un sensor de sonido KY-037 y un display OLED SSD1306 (128x64, I2C). Muestra waveform en tiempo real con detección de trigger configurable y ventana de tiempo ajustable.

- **Lenguaje:** C (C11), C++17 en CMake
- **SDK:** Pico SDK (CMake 3.13+)
- **Firmware:** USB CDC + ADC + I2C OLED
- **Versión actual:** v1.3.1
- **Licencia:** MIT
- **Plataforma:** RP2040 (Dual Core ARM Cortex-M0+, 125 MHz)

---

## 2. Arquitectura del Firmware

```
┌─────────────────────────────────────────────────┐
│                   main.c                         │
│                                                  │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐  │
│  │ ADC Loop │  │ Trigger  │  │ Serial Parser │  │
│  │  ~250Hz  │  │ Detector │  │   (USB CDC)   │  │
│  └────┬─────┘  └────┬─────┘  └───────┬───────┘  │
│       │              │                │           │
│  ┌────▼──────────────▼────────────────▼───────┐  │
│  │         Scope Buffer (128 samples)         │  │
│  └─────────────────┬──────────────────────────┘  │
│                    │                             │
│  ┌─────────────────▼──────────────────────────┐  │
│  │           Display Renderer                  │  │
│  │     (SSD1306 driver + font + drawing)       │  │
│  └─────────────────┬──────────────────────────┘  │
│                    │                             │
│  ┌─────────────────▼──────────────────────────┐  │
│  │         I2C0 (400kHz) → OLED               │  │
│  └────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘

USB CDC ←→ PC (configuración + debug)
```

### Bucle principal (`while(1)`)

| Paso | Función | Frecuencia |
|------|---------|------------|
| 1 | `tud_task()` — procesa USB | Cada iteración |
| 2 | `check_serial_commands()` — lee comandos CDC | Cada iteración |
| 3 | `adc_sample()` — muestrea ADC + llena buffer | Dinámico (`sample_interval_ms`) |
| 4 | `detect_trigger()` — detección de flanco | Cada muestreo |
| 5 | Render OLED + `render()` | Cada 200ms |
| 6 | Monitor serial (si habilitado) | Cada 1000ms |

---

## 3. Mapa de Archivos Detallado

```
pico_src/adc_oled/
├── CMakeLists.txt              # Build raíz: define PICO_SDK_PATH, incluye src/
├── pico_sdk_import.cmake       # Importador del SDK (no modificar, viene del SDK)
├── configure_adc_oled.sh       # Script interactivo de config por serial (bash)
├── install_deps.sh             # Instala dependencias del sistema (sudo)
├── flash_adc_oled.sh           # Compila + flashea con picotool
├── .gitignore                  # Ignora build/, *.o, *.uf2, etc.
├── src/
│   ├── CMakeLists.txt          # Build del ejecutable: target "adc_oled"
│   ├── main.c                  # ★ Código principal (~550 líneas)
│   ├── usb_descriptors.c       # Descriptores USB CDC (device, config, strings)
│   ├── usb_descriptors.h       # Header vacío (guard vacío)
│   ├── tusb_config.h           # Configuración TinyUSB (CDC habilitado)
│   ├── ssd1306_font.h          # Fuente bitmap 8x8 vertical (A-Z, 0-9)
│   └── nlohmann/               # Librería JSON (NO utilizada actualmente)
│       ├── json.hpp            # Header-only JSON (C++)
│       ├── json.h              # Wrapper C
│       └── json.hpp.bak        # Backup
└── docs/
    ├── README.md               # Índice de documentación
    ├── BUILD.md                # Guía de compilación y flasheo
    ├── HARDWARE.md             # Diagramas de conexión (extenso)
    ├── MANUAL_USO.md           # Manual de usuario final
    └── skill.md                # Este archivo
```

---

## 4. Análisis de `main.c` (Código Principal)

### 4.1 Includes y Dependencias

```c
#include "pico/stdlib.h"        // Core: stdio_init_all, sleep_ms, etc.
#include "pico/binary_info.h"   // Binary info para picotool
#include "hardware/adc.h"       // Driver ADC del RP2040
#include "hardware/gpio.h"      // Control de GPIO
#include "hardware/i2c.h"       // Driver I2C del RP2040
#include "bsp/board_api.h"      // board_millis(), etc.
#include "tusb.h"               // TinyUSB device stack
#include "ssd1306_font.h"       // Fuente bitmap para OLED
```

### 4.2 Variables Globales y Estado

| Variable | Tipo | Valor Default | Descripción |
|----------|------|---------------|-------------|
| `current_adc_value` | `uint16_t` | 0 | Último valor ADC leído |
| `current_voltage` | `float` | 0.0 | Voltaje calculado |
| `ky037_digital_state` | `bool` | false | Estado del pin DO |
| `oled_ok` | `bool` | false | Si el OLED fue detectado |
| `trigger_voltage_mv` | `uint16_t` | 1700 | Trigger en mV (1.7V) |
| `trigger_scale` | `float` | 10.0 | Escala/amplitud |
| `trigger_adc_level` | `uint16_t` | 0 | Nivel trigger en counts ADC |
| `monitor_enabled` | `bool` | false | Debug serial activo |
| `trigger_mode` | `uint8_t` | 0 | 0=LOW, 1=HIGH |
| `scope_window_ms` | `uint16_t` | 500 | Ventana de tiempo |
| `scope_buffer[128]` | `uint16_t[]` | — | Buffer circular de muestras |
| `scope_index` | `uint8_t` | 0 | Índice actual en buffer |
| `scope_triggered` | `bool` | false | Si se disparó el trigger |
| `sample_interval_ms` | `uint16_t` | 4 | Intervalo de muestreo dinámico |
| `adc_sum` / `adc_samples` | `uint16_t` | 0 | Acumulador para promedio |
| `noise_floor` | `uint16_t` | 0 | Piso de ruido (se actualiza) |
| `last_trigger_ms` | `uint32_t` | 0 | Timestamp del último trigger |

### 4.3 Flujo de Inicialización (`main()`)

```
1.  stdio_init_all()
2.  cdc_init() → tusb_init() + USB device stack
3.  sleep_ms(500) → esperar USB
4.  cdc_send_string() → banner de inicio por serial
5.  adc_init_sensor() → ADC + GPIO 26 + GPIO 27 + LED
6.  sample_interval_ms = scope_window_ms / 128
7.  i2c_init(400kHz) + GPIO 16/17 como I2C + pull-ups
8.  Scan I2C 0x3C–0x3D → detectar OLED
9.  SSD1306_init() → secuencia de comandos de inicialización
10. Render pantalla de inicio ("KY-037 OSC / Iniciando...")
11. Entrar al bucle principal while(1)
```

### 4.4 Algoritmo de Muestreo y Buffer

```
Cada sample_interval_ms (~4ms):
  adc_read() → raw (0-4095)
  current_voltage = (raw * 3.3) / 4095
  scope_buffer[scope_index] = raw
  scope_index = (scope_index + 1) % 128   ← buffer circular
  adc_sum += raw; adc_samples++           ← para promedio
```

### 4.5 Dos Modos de Visualización

**Modo Rolling (sin trigger):**
- Muestra las 128 muestras más recientes en orden circular
- El display se actualiza como una "cinta transportadora"

**Modo Trigger (post-trigger):**
- Al detectar flanco, se limpia el buffer y se llena desde index 0
- Muestra la waveform desde el punto de trigger hacia la derecha

### 4.6 Detección de Noise Floor

```c
if (current_adc_value < noise_floor)
    noise_floor = current_adc_value;    // Bajar el piso
else if (noise_floor < trigger_adc_level)
    noise_floor++;                       // Subir gradualmente
```

El noise floor se adapta dinámicamente: baja rápido cuando hay señales bajas, sube lentamente hacia el nivel de trigger.

### 4.7 USB CDC Callbacks

| Callback | Acción |
|----------|--------|
| `tud_mount_cb()` | Envía `[USB] Device mounted` |
| `tud_umount_cb()` | Envía `[USB] Device unmounted` |
| `tud_suspend_cb()` | Envía `[USB] Device suspended` |
| `tud_resume_cb()` | Envía `[USB] Device resumed` |

---

## 5. Driver SSD1306 (OLED)

### 5.1 Comandos de Inicialización

El driver SSD1306 está integrado directamente en `main.c` (no es un archivo separado). La secuencia de init envía 19 comandos I2C:

```c
SSD1306_SET_DISP | 0x00        // Display OFF
SSD1306_SET_MEM_MODE, 0x00     // Horizontal addressing
SSD1306_SET_DISP_START_LINE    // Start line = 0
SSD1306_SET_SEG_REMAP | 0x01  // Segment remap (rotado)
SSD1306_SET_MUX_RATIO, 63     // MUX ratio = 64-1
SSD1306_SET_COM_OUT_DIR | 0x08 // COM scan direction
SSD1306_SET_DISP_OFFSET, 0x00  // No offset
SSD1306_SET_COM_PIN_CFG, 0x12  // COM pins (128x64)
SSD1306_SET_DISP_CLK_DIV, 0x80 // Clock divide
SSD1306_SET_PRECHARGE, 0xF1    // Pre-charge period
SSD1306_SET_VCOM_DESEL, 0x30   // VCOMH deselect
SSD1306_SET_CONTRAST, 0xFF     // Contraste máximo
SSD1306_SET_ENTIRE_ON           // Follow RAM content
SSD1306_SET_NORM_DISP           // Normal (no invertido)
SSD1306_SET_CHARGE_PUMP, 0x14  // Charge pump ON
SSD1306_SET_SCROLL | 0x00      // Scroll OFF
SSD1306_SET_DISP | 0x01        // Display ON
```

### 5.2 Funciones del Driver

| Función | Descripción | Líneas approx |
|---------|-------------|---------------|
| `SSD1306_send_cmd(cmd)` | Envía 1 byte de comando (0x80 prefix) | ~3 |
| `SSD1306_send_cmd_list(buf, n)` | Envía lista de comandos | ~4 |
| `SSD1306_send_buf(buf, len)` | Envía buffer de datos (0x40 prefix) | ~7 |
| `SSD1306_init()` | Secuencia completa de inicialización | ~25 |
| `SSD1306_scroll(on)` | Activa/desactiva scroll horizontal | ~10 |
| `render(buf, area)` | Setea dirección de memoria + envía buffer | ~8 |
| `calc_render_area_buflen(area)` | Calcula tamaño del buffer de render | ~3 |

### 5.3 Protocolo I2C

```
Comando:  [0x80, cmd_byte]
Datos:    [0x40, data_byte_1, data_byte_2, ...]
```

- Dirección I2C: 0x3C (o 0x3D)
- Velocidad: 400 kHz (fast mode)
- Sin pull-ups externos (usa los internos del Pico)

### 5.4 Funciones de Dibujo

| Función | Algoritmo | Descripción |
|---------|-----------|-------------|
| `SetPixel(buf, x, y, on)` | Directo a byte | Pixel en buffer page-based |
| `DrawLine(buf, x0,y0, x1,y1, on)` | **Bresenham** | Línea entre dos puntos |
| `DrawCircle(buf, x0,y0, r, filled)` | **Midpoint circle** | Círculo vacío o relleno |
| `WriteChar(buf, x, y, ch)` | Lookup table | Carácter 8x8 desde font[] |
| `WriteString(buf, x, y, str)` | Iterativo | Cadena de caracteres |

### 5.5 Buffer de Display

```c
#define SSD1306_BUF_LEN  (8 * 128)  // 1024 bytes
uint8_t buf[SSD1306_BUF_LEN];       // Frame buffer completo
```

- Organización: 8 páginas × 128 columnas
- Cada byte = 8 pixels verticales
- Se limpia con `memset(buf, 0, SSD1306_BUF_LEN)` antes de cada frame

---

## 6. Fuente Bitmap (`ssd1306_font.h`)

- **Formato:** Vertical bitmaps, 8×8 pixels por carácter
- **Caracteres:** A–Z (mayúsculas), 0–9
- **Codificación:** Cada carácter = 8 bytes, un byte por fila
- **Total:** 37 caracteres × 8 bytes = 296 bytes
- **Index:** `GetFontIndex(ch)` → A=1, B=2, ..., Z=26, 0=27, 1=28, ..., 9=36
- **Carácter 0 (índice 0):** Vacío (todos ceros)
- **Uso:** `WriteChar()` convierte a mayúscula y busca en la tabla

---

## 7. Configuración USB (`tusb_config.h`)

| Constante | Valor | Descripción |
|-----------|-------|-------------|
| `BOARD_TUD_RHPORT` | 0 | Puerto USB principal |
| `CFG_TUD_CDC` | **1** | ✅ CDC habilitado |
| `CFG_TUD_MSC` | 0 | ❌ Mass storage deshabilitado |
| `CFG_TUD_HID` | 0 | ❌ HID deshabilitado |
| `CFG_TUD_MIDI` | 0 | ❌ MIDI deshabilitado |
| `CFG_TUD_VENDOR` | 0 | ❌ Vendor deshabilitado |
| `CFG_TUD_CDC_RX_BUFSIZE` | 64 (USB FS) | Buffer RX CDC |
| `CFG_TUD_CDC_TX_BUFSIZE` | 64 (USB FS) | Buffer TX CDC |
| `CFG_TUD_ENDPOINT0_SIZE` | 64 | Tamaño endpoint 0 |
| `CFG_TUSB_OS` | OPT_OS_NONE | Sin RTOS |
| `CFG_TUSB_MEM_ALIGN` | `__attribute__((aligned(4)))` | Alineación 4 bytes |

---

## 8. Descriptores USB (`usb_descriptors.c`)

### 8.1 Device Descriptor

| Campo | Valor | Descripción |
|-------|-------|-------------|
| `idVendor` | `0xCafe` | Vendor ID (dummy/ejemplo) |
| `idProduct` | `0x4016` | Product ID único |
| `bcdUSB` | `0x0200` | USB 2.0 |
| `bMaxPacketSize0` | 64 | Endpoint 0 |
| `bNumConfigurations` | 1 | Una configuración |

### 8.2 Configuration Descriptor

```
ITF_NUM_CDC     = 0    ← Interface CDC (control)
ITF_NUM_CDC_DATA = 1   ← Interface CDC (data)
EPNUM_CDC_NOTIF = 0x81 ← Endpoint IN  (notificación)
EPNUM_CDC_OUT   = 0x02 ← Endpoint OUT (recepción)
EPNUM_CDC_IN    = 0x82 ← Endpoint IN  (transmisión)
```

- Total config: `TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN`
- Power: 100 mA
- Remote wakeup habilitado

### 8.3 String Descriptors

| Índice | Cadena |
|--------|--------|
| 0 | `0x0409` (English US) |
| 1 | `"Pico ADC+OLED"` (Manufacturer) |
| 2 | `"ADC+OLED+KY037"` (Product) |
| 3 | `"1234567890"` (Serial Number fijo) |

### 8.4 Conversión de Strings

Las strings USB deben ser UTF-16. La función `tud_descriptor_string_cb()` convierte ASCII→UTF-16 dinámicamente, con límite de 31 caracteres.

---

## 9. Comandos Seriales (USB CDC)

### 9.1 Parser de Comandos

```c
static void check_serial_commands(void) {
    // Lee carácter por carácter de tud_cdc_read_char()
    // Acumula en cmd_buf[64]
    // Al recibir \r o \n → procesa comando
}
```

- Buffer: 64 bytes máximo por comando
- Terminador: `\r` o `\n`
- Sin eco de caracteres

### 9.2 Tabla de Comandos

| Comando | Parámetro | Rango | Descripción | Variable modificada |
|---------|-----------|-------|-------------|---------------------|
| `TRIG:voltaje` | float | 0.000–3.300 | Nivel de trigger | `trigger_voltage_mv`, `trigger_adc_level` |
| `TRIGMODE:modo` | 0 o 1 | — | 0=ACTIVE LOW, 1=ACTIVE HIGH | `trigger_mode` |
| `SCALE:valor` | float | 0.1–100.0 | Escala/amplitud | `trigger_scale` |
| `TIME:ms` | uint16 | 100–5000 | Ventana de tiempo | `scope_window_ms`, `sample_interval_ms` |
| `MONITOR:ON/OFF` | string | — | Debug por serial | `monitor_enabled` |
| `GET` | — | — | Config actual (no modifica) | — |
| `VERSION` | — | — | Versión (no modifica) | — |

### 9.3 Formato de Respuesta

```
[CFG] Trigger set to 1.700V (2123 ADC)\r\n
[CFG] Error: voltage must be 0.000 - 3.300V\r\n
[VERSION] adc_oled v1.2.0\r\n
[CFG] Trigger=1.700V (2123 ADC) | Mode=LOW | Scale=10.0 | Window=500ms | Monitor=OFF | NoiseFloor=1234\r\n
```

### 9.4 Ejemplo de Envío (bash)

```bash
echo "TRIG:1.700" > /dev/ttyACM0
echo "SCALE:50" > /dev/ttyACM0
echo "TIME:1000" > /dev/ttyACM0
echo "TRIGMODE:1" > /dev/ttyACM0
echo "MONITOR:ON" > /dev/ttyACM0
echo "GET" > /dev/ttyACM0
echo "VERSION" > /dev/ttyACM0
```

---

## 10. Pantalla OLED (Layout)

```
┌──────────────────────────────────────────────────────────┐
│ V:1.650V      T:1.700V 500ms                      ●     │  ← y=0, page 0
│ (ADC avg)     (trigger level + window)          (DO)    │
│──────────────────────────────────────────────────────────│
│                                                          │
│                                                          │
│                WAVEFORM / OSCILLOSCOPE                    │  ← y=16..63
│                (128 samples, connected lines)             │     (48 px alto)
│                                                          │
│  ...........  (trigger level dashed, every 4 px)         │
│                                                          │
└──────────────────────────────────────────────────────────┘
            x=0                                    x=127
```

### Zonas de la pantalla

| Zona | Coordenadas | Contenido |
|------|-------------|-----------|
| Header izq | x=0, y=0 | `V:XXXX V` — voltaje promedio |
| Header medio | x=42, y=0 | `T:XXX V XXXms` — trigger + ventana |
| Header der | x=120, y=0 | Círculo DO (4px radio) |
| Waveform | x=0–127, y=16–63 | Trazo de 48px alto |
| Trigger line | y = calculado | Línea punteada (cada 4px) |

### Cálculo de posición Y del waveform

```c
uint8_t y = trace_y + trace_h - ((val * trace_h) / 4095);
// trace_y = 16, trace_h = 48
// Val 0 → y=64 (abajo), Val 4095 → y=16 (arriba)
```

---

## 11. Lógica de Trigger

### 11.1 Detección de Flanco

```c
static bool detect_trigger(uint32_t now) {
    bool armed = (now - last_trigger_ms) > TRIGGER_HOLD_MS;  // 300ms

    if (trigger_mode == 0) {  // ACTIVE LOW
        bool below_level = current_adc_value < trigger_adc_level;
        bool was_above = last_adc_value >= trigger_adc_level;
        trigger_event = below_level && was_above && armed;
    } else {                 // ACTIVE HIGH
        bool above_level = current_adc_value > trigger_adc_level;
        bool was_below = last_adc_value <= trigger_adc_level;
        trigger_event = above_level && was_below && armed;
    }
}
```

### 11.2 Comportamiento Post-Trigger

```c
if (trigger) {
    memset(scope_buffer, 0, sizeof(scope_buffer));  // Limpiar buffer
    scope_index = 0;                                 // Empezar desde 0
    scope_triggered = true;                          // Modo post-trigger
}
```

### 11.3 Conversión Voltaje → ADC

```c
trigger_voltage_mv = (uint16_t)(voltage * 1000.0f);
trigger_adc_level = (uint16_t)((trigger_voltage_mv / 1000.0f) * 4095.0f / 3.3f);
```

---

## 12. Hardware (Conexiones)

| Componente | Pin Pico | GPIO | Función |
|------------|----------|------|---------|
| KY-037 AO  | Pin 33   | GP27 | ADC1 (analógico) |
| KY-037 DO  | Pin 32   | GP26 | Digital input |
| OLED SDA   | Pin 25   | GP16 | I2C0 SDA |
| OLED SCL   | Pin 26   | GP17 | I2C0 SCL |
| LED        | Pin 25   | GP25 | LED integrado del Pico |
| Alimentación | Pin 37/35 | 3V3/GND | Compartido |

- **ADC:** 12 bits (0–4095), referencia 3.3V, solo GPIO 26–29
- **I2C:** 400 kHz, dirección OLED: 0x3C o 0x3D
- **OLED:** SSD1306, 128×64 pixels, 3.3V (sin level shifter)

---

## 13. Scripts

### 13.1 `flash_adc_oled.sh`

Script de compilación + flasheo con 3 modos:

| Flag | Acción |
|------|--------|
| (sin flag) | Compilar + flashear + verificar |
| `--compile-only` | Solo compilar |
| `--flash-only` | Solo flashear |

**Flujo:**
1. Limpia `build/`, ejecuta `cmake ..` + `make -j$(nproc)`
2. Usa `picotool` (local en `/mnt/disk/src/rpico/picotool/build/picotool` o del sistema)
3. Detecta Pico por USB: `lsusb | grep "2e8a:0003"` (BOOTSEL)
4. Si no está en BOOTSEL, intenta `picotool reboot -u`
5. Verifica dispositivo CDC: `lsusb | grep "cafe:4016"`

**Output esperado:** `build/src/adc_oled.uf2`

### 13.2 `configure_adc_oled.sh`

Script interactivo de configuración por serial:

1. Detecta puerto serie: `/dev/ttyACM0` a `/dev/ttyUSB1`
2. Configura `stty 115200 raw -echo`
3. Menú interactivo con opciones:
   - Configurar trigger (TRIG)
   - Configurar escala (SCALE)
   - Configurar modo trigger (TRIGMODE)
   - Ver configuración (GET)
   - Ver versión (VERSION)
4. Reintentos automáticos (3 intentos por comando)
5. Filtra respuestas: solo muestra líneas `[CFG]` o `[VERSION]`

### 13.3 `install_deps.sh`

Instalador de dependencias (requiere `sudo`):

| Paquete | Propósito |
|---------|-----------|
| `build-essential` | Compilador C, make |
| `cmake` | Sistema de build |
| `gcc-arm-none-eabi` | Compilador ARM cross-compilation |
| `libnewlib-arm-none-eabi` | C library para ARM |
| `git` | Control de versiones |
| `python3` | Herramientas del SDK |
| `libusb-1.0-0-dev` | Para picotool |

Verifica también la existencia de `picotool` compilado.

---

## 14. Build System (CMake)

### 14.1 `CMakeLists.txt` (raíz)

```cmake
cmake_minimum_required(VERSION 3.13)
set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../pico-sdk")
include(pico_sdk_import.cmake)
project(adc_oled C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_subdirectory(src)
```

### 14.2 `src/CMakeLists.txt`

```cmake
add_executable(adc_oled main.c usb_descriptors.c)
target_include_directories(adc_oled PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(adc_oled PUBLIC
    pico_stdlib         # Core SDK
    hardware_adc        # Driver ADC
    hardware_i2c        # Driver I2C
    tinyusb_device      # USB device stack
    tinyusb_board       # Board support USB
)
pico_add_extra_outputs(adc_oled)  # Genera .uf2, .bin, .elf, .map
```

### 14.3 Archivos Generados

```
build/
├── adc_oled.uf2      ← Para flashear (drag & drop en BOOTSEL)
├── adc_oled.elf      ← Con debug info (para gdb)
├── adc_oled.bin      ← Binario raw
├── adc_oled.elf.map  ← Mapa de símbolos
└── adc_oled.dis      ← Disassembly
```

---

## 15. Constantes Clave

```c
// === ADC ===
#define ADC_PIN            27      // GPIO 27 (ADC1)
#define ADC_CHANNEL        1       // Canal ADC 1
#define ADC_VREF           3.3f    // Voltaje referencia
#define ADC_RESOLUTION     4095.0f // 12 bits

// === I2C ===
#define I2C_PORT           i2c0    // Usar I2C0
#define I2C_SDA_PIN        16      // GPIO 16
#define I2C_SCL_PIN        17      // GPIO 17
#define I2C_BAUDRATE       400000  // 400 kHz fast mode

// === OLED ===
#define SSD1306_HEIGHT     64
#define SSD1306_WIDTH      128
#define SSD1306_I2C_ADDR   0x3C
#define SSD1306_BUF_LEN    1024    // 8 pages × 128 cols

// === TIMING ===
#define DISPLAY_UPDATE_MS  200     // Refresh OLED
#define SERIAL_UPDATE_MS   1000    // Monitor serial
#define TRIGGER_HOLD_MS    300     // Anti-retrigger

// === OSCILSCOPIO ===
#define SCOPE_BUFFER_SIZE  128     // Muestras en buffer
#define KY037_DO_PIN       26      // Digital input
static uint16_t scope_window_ms = 500;   // Ventana default
static uint16_t sample_interval_ms = 4;  // ~250 Hz
```

---

## 16. Mapeo GPIO → ADC

| GPIO | Canal ADC | Uso en este proyecto |
|------|-----------|---------------------|
| GP26 | ADC0 | KY-037 DO (digital) |
| GP27 | ADC1 | ✅ KY-037 AO (analógico) |
| GP28 | ADC2 | Disponible |
| GP29 | ADC3 | Disponible |

**Nota:** GP26 también puede ser ADC0, pero en este proyecto se usa como digital input.

---

## 17. nlohmann JSON (No Utilizado)

La carpeta `src/nlohmann/` contiene la librería JSON de nlohmann:
- `json.hpp` — Header-only C++ JSON library
- `json.h` — Wrapper C
- `json.hpp.bak` — Backup

**Estado:** No está incluida en `main.c` ni en el build. Parece preparada para funcionalidad futura (posiblemente exportación de datos por USB).

---

## 18. Troubleshooting

| Problema | Causa probable | Solución |
|----------|----------------|----------|
| OLED no enciende | Conexión I2C mala | Verificar SDA/SCL, dirección 0x3C/0x3D |
| OLED sin imagen | Driver no inicializado | Verificar `SSD1306_init()` y contraste (0xFF) |
| KY-037 no detecta | Sensibilidad baja | Ajustar potenciómetro del KY-037 |
| ADC no cambia | Pin incorrecto | Verificar GPIO 27 (ADC1, no ADC0) |
| USB no aparece | BOOTSEL no presionado | Repetir procedimiento BOOTSEL |
| Build falla: PICO_SDK_PATH | SDK no encontrado | Verificar `../../pico-sdk` existe |
| Build falla: arm-none-eabi | Compiler no instalado | `sudo apt install gcc-arm-none-eabi` |
| Errores TinyUSB | Config incorrecta | Verificar `tusb_config.h` — solo CDC habilitado |
| Trigger no dispara | Nivel muy bajo/alto | Verificar `TRIG:voltaje` en rango 0–3.3V |
| Display parpadea | I2C inestable | Usar cables cortos, pull-ups, reducir a 100kHz |
| `picotool` no encontrado | No compilado | Compilar desde `/mnt/disk/src/rpico/picotool` |
| Puerto serial no aparece | Pico no montado como CDC | Verificar firmware cargado correctamente |

---

## 19. Dependencias del Build

| Dependencia | Ubicación | Uso |
|-------------|-----------|-----|
| Pico SDK | `../../pico-sdk` | Core del SDK |
| TinyUSB | `pico-sdk/lib/tinyusb` | USB CDC device stack |
| hardware_adc | Pico SDK | Driver ADC RP2040 |
| hardware_i2c | Pico SDK | Driver I2C RP2040 |
| pico_stdlib | Pico SDK | Funciones básicas |
| tinyusb_device | Pico SDK | USB device |
| tinyusb_board | Pico SDK | Board support |

---

## 20. Extensibilidad

### Agregar nuevo comando serial

1. Editar `process_serial_command()` en `main.c`
2. Agregar bloque `else if (strncmp(cmd, "NUEVO:", 6) == 0)`
3. Responder con `cdc_send_string("[CFG] ...\r\n")`
4. Documentar en este archivo

### Cambiar pin ADC

```c
// En main.c, cambiar:
#define ADC_PIN    28    // Nuevo GPIO
#define ADC_CHANNEL 2    // Canal correspondiente
```

### Agregar segundo canal ADC

- Leer GP28 (ADC2) en paralelo
- Agregar segundo buffer o mostrar ambos traces
- Necesita modificar el loop de muestreo y el render

### Agregar botones hardware

- Usar GPIO libres: GP7–GP15, GP18–GP22
- Pull-up interno, detectar LOW = presionado
- Cambiar modo de visualización o trigger

---

## 21. Diagrama de Estados

```
                    ┌──────────────────┐
                    │   INICIALIZACIÓN  │
                    │  USB + ADC + I2C  │
                    └────────┬─────────┘
                             │
                             ▼
                    ┌──────────────────┐
                    │   LOOP PRINCIPAL  │◄─────────────┐
                    │  tud_task()       │              │
                    │  check_serial()   │              │
                    └────────┬─────────┘              │
                             │                         │
              ┌──────────────┼──────────────┐          │
              ▼              ▼              ▼          │
     ┌────────────┐  ┌────────────┐  ┌────────────┐   │
     │  MUESTREO  │  │  TRIGGER   │  │   RENDER   │   │
     │  ADC 250Hz │  │  DETECT    │  │  OLED 5Hz  │   │
     └──────┬─────┘  └──────┬─────┘  └──────┬─────┘   │
            │               │               │          │
            │          ┌────▼────┐          │          │
            │          │ TRIGGER │          │          │
            │          │  FIRE!  │          │          │
            │          │ Limpiar │          │          │
            │          │ buffer  │          │          │
            │          └─────────┘          │          │
            │                               │          │
            └───────────────────────────────┴──────────┘
```
