# MIDI Touch Pads - Raspberry Pi Pico USB MIDI Controller

Convierte tu Raspberry Pi Pico en un controlador MIDI USB con touch pads analógicos.

## Funcionalidad

- Lee 4 touch pads conectados a pines ADC (GPIO 26, 27, 28 + sensor interno)
- Detecta toque vs. no-toque con debouncing
- Envía **Note On/Off** y **Control Change** via USB MIDI
- La velocidad (velocity) se calcula según la intensidad del toque

## Mapeo de Pads a Notas MIDI

| Pad | GPIO | ADC Canal | Nota MIDI | Nota Musical |
|-----|------|-----------|-----------|--------------|
| 1   | 26   | ADC0      | 60        | C4           |
| 2   | 27   | ADC1      | 62        | D4           |
| 3   | 28   | ADC2      | 64        | E4           |
| 4   | -    | ADC4 (temp) | 67     | G4           |

## Circuito de Touch Pad (método simple)

Conecta cada pad a través de un resistor de 1MΩ al pin ADC, y una lámina/hilo conductor al GPIO. Cuando tocas la lámina, tu cuerpo actúa como capacitancia al suelo, bajando el voltaje.

```
Touch Pad ──┬── 1MΩ resistor ──── GPIO 26 (ADC0)
            │
            └── 3.3V (con pull-up interno o externo)
```

**Otro método (más fácil):** Usa una lámina de cobre/aluminio directamente conectada al GPIO con un resistor de 10kΩ a 3.3V. ADC lee el voltaje; al tocar, la capacitancia del cuerpo baja el valor.

## Compilación

```bash
cd my-pico-projects
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

El archivo `.uf2` se genera en: `build/midi-touch-pads/midi_touch_pads.uf2`

## Flasheo

1. Mantener presionado el botón BOOTSEL en el Pico
2. Conectar el Pico al USB (aparece como unidad BOOT)
3. Copiar el archivo `.uf2` a la unidad:
   ```bash
   cp build/midi-touch-pads/midi_touch_pads.uf2 /media/$USER/RP2040/
   ```
4. El Pico se reinicia y aparece como dispositivo MIDI USB

## Uso como MIDI

### Linux
```bash
# Instalar qsynth (sintetizador) y qjackctl (conexiones MIDI)
sudo apt install qsynth qjackctl

# Abrir qjackctl, conectar "Pico Touch MIDI" a "FLUID Synth Input"
```

### Windows
- Instalar [MIDI-OX](https://www.midiox.com/) o usar cualquier DAW (Ableton, FL Studio, etc.)
- El dispositivo aparece como "Pico Touch MIDI"

### macOS
- Usar [SimpleSynth](https://www.notahat.com/simplesynth/) o GarageBand
- El dispositivo se detecta automáticamente

## Ajustes

Edita `main.c` para cambiar:

```c
#define TOUCH_THRESHOLD  1500   // Umbral ADC (0-4095)
#define MIN_VELOCITY      30    // Velocidad mínima
#define MAX_VELOCITY     127    // Velocidad máxima
#define DEBOUNCE_COUNT    3     // Anti-rebote
#define SCAN_INTERVAL_MS  10    // Frecuencia de escaneo (ms)

// Notas MIDI:
static const uint8_t midi_notes[NUM_PADS] = { 60, 62, 64, 67 };
```

## Estructura del Proyecto

```
my-pico-projects/
├── CMakeLists.txt              # CMake principal
├── pico_sdk_import.cmake       # Importador del SDK
└── midi-touch-pads/
    ├── CMakeLists.txt          # Config del proyecto
    ├── main.c                  # Lógica principal (ADC + MIDI)
    ├── tusb_config.h           # Configuración TinyUSB (MIDI habilitado)
    ├── usb_descriptors.c       # Descriptores USB para MIDI
    └── usb_descriptors.h       # Header de descriptores
```

## Agregar Nuevos Proyectos

1. Crear carpeta en `my-pico-projects/`
2. Crear `CMakeLists.txt` con `add_executable(...)` y `target_link_libraries(... pico_stdlib)`
3. Agregar `add_subdirectory(tu-proyecto)` en el `CMakeLists.txt` principal
4. Crear `tusb_config.h` local si se necesita USB

## Licencia

MIT
