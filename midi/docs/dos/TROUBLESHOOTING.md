# TROUBLESHOOTING - Solución de Problemas

## Problemas de Build

### "PICO_SDK_PATH not set"
```bash
# Solución: exportar la variable
export PICO_SDK_PATH=/ruta/a/pico-sdk

# O configurar en CMakeLists.txt:
# set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../pico-sdk")
```

### "arm-none-eabi-gcc: command not found"
```bash
# Ubuntu/Debian
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi

# macOS (Homebrew)
brew install --cask gcc-arm-embedded

# Verificar instalación
arm-none-eabi-gcc --version
```

### "TinyUSB not found"
```bash
# Verificar si TinyUSB existe
ls pico-sdk/lib/tinyusb/

# Si no existe, inicializar submodule
cd pico-sdk
git submodule update --init lib/tinyusb
```

### "No rule to make target 'tusb_config.h'"
Asegúrate que `tusb_config.h` está en la misma carpeta que `main.c`:
```bash
ls pico_projects/midi/tusb_config.h
```

### Build falla con "undefined reference"
Rebuild limpio:
```bash
cd pico_projects/build
rm -rf *
cmake ..
make -j$(nproc)
```

---

## Problemas de Hardware

### Los pads no detectan toque

**Causa 1:** Resistores incorrectos
- Verificar que son **1MΩ** (no 1kΩ ni 10kΩ)
- Medir con multímetro

**Causa 2:** Conexiones sueltas
- Verificar soldaduras
- Probar con cables diferentes

**Causa 3:** Umbral muy bajo
- Aumentar `TOUCH_THRESHOLD` en `main.c`:
```c
// Cambiar de 1500 a 2000 o 2500
#define TOUCH_THRESHOLD  2000
```

**Causa 4:** GPIO incorrecto
- Verificar que usas GPIO 26, 27, 28 (pines 4, 3, 2)
- NO usar GPIO 29 (mide voltaje de batería)

### Los pads detectan sin tocar

**Causa 1:** Resistores faltantes
- Asegúrate que cada pad tiene resistor de 1MΩ a 3.3V

**Causa 2:** Umbral muy alto
- Reducir `TOUCH_THRESHOLD`:
```c
#define TOUCH_THRESHOLD  1000
```

**Causa 3:** Interferencia
- Alejar láminas del cable USB
- Usar cable USB blindado
- Agregar capacitor de 100nF entre 3.3V y GND

### Velocity muy sensible o muy poco

Ajustar rango en `main.c`:
```c
// Mínimo más alto = más sensible
#define MIN_VELOCITY   50

// Máximo más bajo = menos rango
#define MAX_VELOCITY   100
```

---

## Problemas de MIDI

### El PC no detecta el dispositivo MIDI

**Windows:**
1. Abrir Administrador de dispositivos
2. Buscar "Dispositivos de sonido"
3. Si no aparece, reinstalar driver USB
4. Probar otro puerto USB

**macOS:**
1. Abrir Audio MIDI Setup
2. Verificar que "Pico Touch MIDI" aparece
3. Si no aparece, reiniciar el Mac

**Linux:**
```bash
# Verificar dispositivo USB
lsusb | grep -i cafe

# Verificar ALSA
arecord -l | grep -i midi

# Verificar kernel messages
dmesg | tail -20
```

### Las notas suenan pero sin velocity

- Verificar que el sintetizador soporta velocity
- Probar con MIDI-OX para ver los mensajes raw
- Verificar que `MIN_VELOCITY` no es 0

### Latencia alta

- Reducir `SCAN_INTERVAL_MS`:
```c
#define SCAN_INTERVAL_MS  5  // de 10 a 5 ms
```
- Usar USB 2.0 (puerto azul si es hub)
- Cerrar otros programas que usen MIDI

### Notas "pegadas" (no sueltan)

- Verificar debounce: aumentar `DEBOUNCE_COUNT`:
```c
#define DEBOUNCE_COUNT  5
```
- Verificar que `tud_midi_stream_write` se llama correctamente
- Reiniciar el Pico

---

## Problemas de Flasheo

### El Pico no entra en modo BOOTSEL

1. **Desconectar** el USB
2. **Mantener** BOOTSEL presionado
3. **Conectar** USB mientras mantienes BOOTSEL
4. **Esperar** 2 segundos antes de soltar

### El .uf2 no copia

- Verificar que la unidad se llama `RPI-RP2` o `RP2040`
- En Windows, probar otro puerto USB
- En Linux, verificar permisos:
```bash
sudo chmod -R 777 /media/$USER/RPI-RP2/
```

### El Pico no arranca después del flashear

1. Desconectar USB
2. Esperar 3 segundos
3. Reconectar
4. Si no funciona, repetir flasheo

---

## Problemas de Reconexión

### El Pico no aparece después de cambiar código

1. Abrir Administrador de dispositivos (Windows) o `dmesg` (Linux)
2. Si aparece como "Unknown device":
   - Desinstalar dispositivo
   - Desconectar y reconectar USB
3. Probar otro cable USB
4. Probar otro puerto USB

### El Pico se desconecta solo

- Verificar consumo de corriente (máximo 500mA USB)
- No usar hubs sin alimentación
- Verificar que no hay cortocircuitos

---

## Logs de Debug

Para habilitar logs por USB CDC, agregar al inicio de `main.c`:
```c
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();  // Agregar esta línea
    // ... resto del código
}
```

Luego usar `printf()`:
```c
printf("ADC raw: %d\n", raw);
printf("Velocity: %d\n", pads[i].velocity);
printf("Note On: %d\n", midi_notes[i]);
```

Para ver los logs:
- **Linux:** `screen /dev/ttyACM0 115200`
- **macOS:** `screen /dev/cu.usbmodem* 115200`
- **Windows:** PuTTY → COM port → 115200 baud

**Nota:** USB CDC puede interferir con MIDI. Para producción, quitar `stdio_init_all()`.
