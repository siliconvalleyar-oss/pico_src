# SKILL.md - Osciloscopio Digital KY-037 (Raspberry Pi Pico)

Referencia técnica completa del proyecto **adc_oled**. Este archivo está diseñado para que un agente de IA o desarrollador pueda comprender, modificar y mantener el proyecto de forma autónoma.

---

## 1. Resumen del Proyecto

Osciloscopio digital basado en Raspberry Pi Pico RP2040 que usa un sensor de sonido KY-037 y un display OLED SSD1306 (128x64, I2C). Muestra waveform en tiempo real con detección de trigger configurable.

- **Lenguaje:** C (C11)
- **SDK:** Pico SDK (CMake)
- **Firmware:** USB CDC + ADC + I2C OLED
- **Versión actual:** v1.2.0

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
│  │        (OLED SSD1306 via I2C)               │  │
│  └────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

### Bucle principal (while(1))

1. `tud_task()` — procesa USB
2. `check_serial_commands()` — lee comandos CDC
3. Muestreo ADC cada `sample_interval_ms` (~4ms ≈ 250Hz)
4. Detección de trigger (flanco rising/falling)
5. Actualización de display cada 200ms
6. Monitor serial cada 1s (si habilitado)

---

## 3. Mapa de Archivos

```
pico_src/adc_oled/
├── CMakeLists.txt              # Build raíz, define PICO_SDK_PATH
├── pico_sdk_import.cmake       # Importador del SDK (no modificar)
├── configure_adc_oled.sh       # Script interactivo de config por serial
├── install_deps.sh             # Instala dependencias del sistema
├── flash_adc_oled.sh           # Flashea el .uf2 al Pico
├── src/
│   ├── CMakeLists.txt          # Build del ejecutable (target: adc_oled)
│   ├── main.c                  # ★ Código principal (~550 líneas)
│   ├── usb_descriptors.c/.h    # Descriptores USB CDC
│   ├── tusb_config.h           # Configuración TinyUSB
│   ├── ssd1306_font.h          # Fuente bitmap 8x8 para OLED
│   └── nlohmann/               # JSON (no utilizado actualmente)
└── docs/
    ├── README.md               # Índice de documentación
    ├── BUILD.md                # Guía de compilación y flasheo
    ├── HARDWARE.md             # Diagramas de conexión
    ├── MANUAL_USO.md           # Manual de usuario final
    └── skill.md                # Este archivo
```

---

## 4. Hardware (Conexiones Rápidas)

| Componente | Pin Pico | GPIO | Función |
|------------|----------|------|---------|
| KY-037 AO  | Pin 33   | GP27 | ADC1 (analógico) |
| KY-037 DO  | Pin 32   | GP26 | Digital input |
| OLED SDA   | Pin 25   | GP16 | I2C0 SDA |
| OLED SCL   | Pin 26   | GP17 | I2C0 SCL |
| Alimentación | Pin 37/35 | 3V3/GND | Compartido |

- **ADC:** 12 bits (0–4095), referencia 3.3V
- **I2C:** 400 kHz, dirección OLED: 0x3C o 0x3D
- **OLED:** SSD1306, 128×64 pixels

---

## 5. Constantes Clave (main.c)

```c
// ADC
#define ADC_PIN            27      // GPIO 27
#define ADC_CHANNEL        1       // ADC channel 1
#define ADC_VREF           3.3f
#define ADC_RESOLUTION     4095.0f

// Display
#define SSD1306_HEIGHT     64
#define SSD1306_WIDTH      128
#define SSD1306_I2C_ADDR   0x3C

// Timing
#define DISPLAY_UPDATE_MS  200     // Refresh OLED cada 200ms
#define SERIAL_UPDATE_MS   1000    // Debug serial cada 1s
#define TRIGGER_HOLD_MS    300     // Anti-retrigger 300ms

// Buffer osciloscopio
#define SCOPE_BUFFER_SIZE  128     // 128 muestras en buffer circular
static uint16_t scope_window_ms = 500;  // Ventana default
```

---

## 6. Comandos Seriales (USB CDC)

| Comando | Parámetro | Rango | Descripción |
|---------|-----------|-------|-------------|
| `TRIG:voltaje` | float | 0.000–3.300 | Nivel de trigger en voltios |
| `TRIGMODE:modo` | 0 o 1 | — | 0=ACTIVE LOW (falling), 1=ACTIVE HIGH (rising) |
| `SCALE:valor` | float | 0.1–100.0 | Escala/amplitud |
| `TIME:ms` | uint16 | 100–5000 | Ventana de tiempo en ms |
| `MONITOR:ON/OFF` | string | — | Habilita debug por serial |
| `GET` | — | — | Muestra configuración actual |
| `VERSION` | — | — | Versión del firmware |

**Formato de respuesta:** `[CFG] mensaje` o `[VERSION] mensaje`

**Ejemplo de envío (bash):**
```bash
echo "TRIG:1.700" > /dev/ttyACM0
echo "SCALE:50" > /dev/ttyACM0
echo "GET" > /dev/ttyACM0
```

---

## 7. Pantalla OLED (Layout)

```
┌────────────────────────────────────────────────────┐
│ V:1.650V        T:1.700V 500ms              ●     │  ← y=0
│ (ADC avg)       (trigger)              (DO indicator)
│────────────────────────────────────────────────────│
│                                                    │
│                                                    │
│              WAVEFORM / OSCILLOSCOPE                │  ← y=16..63
│              (128 samples, connected lines)         │
│                                                    │
│ ...............  (trigger level dashed line)       │
│                                                    │
└────────────────────────────────────────────────────┘
          x=0                              x=127
```

- **V:XXXX** — Voltaje ADC promedio (3 decimales)
- **T:XXXX ms** — Nivel de trigger + ventana
- **●** — Círculo lleno = DO HIGH, vacío = DO LOW
- **Waveform** — Trazo conectado con líneas (algoritmo de Bresenham)
- **Línea punteada** — Nivel de trigger

---

## 8. Lógica de Trigger

```c
// Active LOW (trigger_mode == 0): se dispara cuando la señal BAJA
bool below_level = current_adc_value < trigger_adc_level;
bool was_above = last_adc_value >= trigger_adc_level;
trigger_event = below_level && was_above && armed;

// Active HIGH (trigger_mode == 1): se dispara cuando la señal SUBE
bool above_level = current_adc_value > trigger_adc_level;
bool was_below = last_adc_value <= trigger_adc_level;
trigger_event = above_level && was_below && armed;
```

- **Hold time:** 300ms anti-retrigger
- **Al trigger:** se limpia el buffer y se empieza a llenar desde index 0
- **Sin trigger:** el buffer se muestra en modo "rolling" (circular)

---

## 9. Funciones de Dibujo (OLED)

| Función | Descripción |
|---------|-------------|
| `SetPixel(buf, x, y, on)` | Enciende/apaga un pixel |
| `DrawLine(buf, x0,y0, x1,y1, on)` | Línea con algoritmo de Bresenham |
| `DrawCircle(buf, x0,y0, radius, filled)` | Círculo vacío o relleno |
| `WriteChar(buf, x, y, ch)` | Escribe un carácter (fuente 8x8) |
| `WriteString(buf, x, y, str)` | Escribe una cadena de texto |
| `render(buf, area)` | Envia buffer al OLED vía I2C |

---

## 10. Configuración por Serial (configure_adc_oled.sh)

Script bash interactivo que se conecta al puerto serial del Pico y permite:

1. **Configurar trigger** — envía `TRIG:voltaje`
2. **Configurar escala** — envía `SCALE:valor`
3. **Configurar modo trigger** — envía `TRIGMODE:0/1`
4. **Ver configuración** — envía `GET`
5. **Ver versión** — envía `VERSION`

**Dependencias:** `stty`, `cat`, `bc`

---

## 11. Build y Flash

### Compilar
```bash
cd pico_src/adc_oled
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Output
```
build/adc_oled/adc_oled.uf2    ← Para flashear
build/adc_oled/adc_oled.elf    ← Debug
build/adc_oled/adc_oled.bin    ← Binario raw
```

### Flashear
```bash
# Mantener BOOTSEL + conectar USB
cp build/adc_oled/adc_oled.uf2 /media/$USER/RPI-RP2/
```

### Limpiar
```bash
cd build && rm -rf * && cmake .. && make -j$(nproc)
```

---

## 12. Troubleshooting

| Problema | Causa probable | Solución |
|----------|----------------|----------|
| OLED no enciende | Conexión I2C mala | Verificar SDA/SCL, dirección 0x3C/0x3D |
| OLED sin imagen | Driver no inicializado | Verificar `SSD1306_init()` y contraste |
| KY-037 no detecta | Sensibilidad baja | Ajustar potenciómetro del KY-037 |
| ADC no cambia | Pin incorrecto | Verificar GPIO 27 (ADC1) |
| USB no aparece | BOOTSEL no presionado | Repetir procedimiento BOOTSEL |
| Build falla | SDK no encontrado | Verificar `PICO_SDK_PATH` en CMakeLists.txt |
| Errores TinyUSB | Config incorrecta | Verificar `tusb_config.h` |

---

## 13. Dependencias

| Dependencia | Ubicación | Uso |
|-------------|-----------|-----|
| Pico SDK | `../../pico-sdk` | Core del SDK |
| TinyUSB | `pico-sdk/lib/tinyusb` | USB CDC |
| hardware_adc | Pico SDK | Lectura ADC |
| hardware_i2c | Pico SDK | Comunicación I2C OLED |

---

## 14. Extensibilidad

El código está preparado para:

- **Más canales ADC:** GP26 (ADC0), GP27 (ADC1), GP28 (ADC2), GP29 (ADC3)
- **Más comandos seriales:** agregar en `process_serial_command()`
- **Más displays:** reemplazar driver SSD1306
- **Más sensores:** el KY-037 es reemplazable por cualquier sensor analógico

### Para agregar un nuevo comando serial

1. Agregar el handler en `process_serial_command()` dentro de `main.c`
2. Formato: `strncmp(cmd, "NUEVO:", 6) == 0`
3. Responder con `cdc_send_string("[CFG] ...\r\n")`
4. Documentar en este archivo

### Para cambiar el pin ADC

```c
// En main.c:
#define ADC_PIN    28    // CambiarGPIO
#define ADC_CHANNEL 2    // Canal correspondiente
```

GPIO → Canal ADC:
| GPIO | Canal |
|------|-------|
| GP26 | ADC0 |
| GP27 | ADC1 |
| GP28 | ADC2 |
| GP29 | ADC3 |

---

## 15. Licencia

MIT License. Libre para uso personal y comercial.
