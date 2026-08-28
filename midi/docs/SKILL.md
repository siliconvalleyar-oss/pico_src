# 🎹 SKILL: Raspberry Pi Pico MIDI Touch Pads

Guía completa para crear un controlador MIDI USB con touch pads analógicos usando Raspberry Pi Pico.

---

## 📋 Resumen del Proyecto

**Objetivo:** Convertir un Raspberry Pi Pico en un controlador MIDI USB que lea touch pads analógicos y envíe notas musicales a un sintetizador via Bluetooth.

**Componentes:**
- Raspberry Pi Pico (1 o 2W)
- 3 touch pads (láminas de cobre/aluminio)
- 3 resistencias de 1MΩ
- Raspberry Pi (para compilar y ejecutar FluidSynth)
- Speaker Bluetooth (Xiaomi Sound Pocket)

---

## 🔧 Arquitectura del Sistema

```
┌─────────────────┐    USB MIDI    ┌──────────────┐    PulseAudio    ┌─────────────────┐
│   Touch Pads    │ ──────────────→ │  FluidSynth  │ ──────────────→ │ Bluetooth       │
│   (GPIO 26-28)  │                 │  (Sintetizer)│                 │ (Xiaomi Speaker)│
│                 │                 │              │                 │                 │
│   Pico Touch   │                 │  SoundFont:  │                 │ AC:EF:92:D0:    │
│   MIDI         │                 │  General MIDI│                 │ B5:BB           │
└─────────────────┘                 └──────────────┘                 └─────────────────┘
```

---

## 📁 Estructura del Proyecto

```
pico_src/
├── midi/
│   ├── CMakeLists.txt              # Config CMake principal
│   ├── pico_sdk_import.cmake       # Importador SDK
│   ├── src/
│   │   ├── CMakeLists.txt          # Build del firmware
│   │   ├── main.c                  # Código principal
│   │   ├── tusb_config.h           # Config TinyUSB
│   │   ├── usb_descriptors.c       # Descriptores USB MIDI
│   │   └── usb_descriptors.h       # Header
│   └── docs/
│       ├── SKILL.md                # Este archivo
│       ├── QUICK_START.md          # Guía rápida
│       ├── MANUAL_USO.md           # Manual de usuario
│       ├── HARDWARE.md             # Hardware y conexiones
│       ├── BUILD.md                # Compilación
│       ├── MIDI.md                 # Referencia MIDI
│       ├── SCALES.md               # Escalas musicales
│       ├── REMOTE_FLASH.md         # Flasheo remoto
│       └── TROUBLESHOOTING.md      # Solución de problemas
├── script_tools/
│   ├── install_midi_tools.sh       # Instalador completo
│   ├── fix_first_build.sh          # Corregir primer build
│   ├── flash_full.sh               # Compile + flash
│   ├── check_midi.sh               # Verificar MIDI
│   ├── test_midi.py                # Monitorear toques
│   ├── start_bt_synth.py           # Iniciar synth Bluetooth
│   ├── change_instrument.sh        # Cambiar instrumento
│   └── install_deps.sh             # Instalar dependencias
└── README.md                       # Documentación principal
```

---

## 🚀 Setup Inicial (Raspberry Pi)

### 1. Instalar Dependencias

```bash
sudo bash /home/joy/src/pico_src/script_tools/install_midi_tools.sh
```

**Esto instala:**
- gcc-arm-none-eabi (compilador ARM)
- cmake, make, git
- FluidSynth + SoundFont
- PulseAudio + Bluetooth
- Python mido + python-rtmidi
- Pico SDK

### 2. Clonar Repositorio

```bash
cd /home/joy/src
git clone https://github.com/siliconvalleyar-oss/pico_src.git
```

### 3. Verificar Instalación

```bash
bash /home/joy/src/pico_src/script_tools/check_midi.sh
```

---

## 🔌 Hardware - Conexiones

### Touch Pads

```
Lámina 1 ──── GPIO 26 (Pin 32) ──── 1MΩ ──── 3.3V (Pin 37)
Lámina 2 ──── GPIO 27 (Pin 33) ──── 1MΩ ──── 3.3V (Pin 37)
Lámina 3 ──── GPIO 28 (Pin 34) ──── 1MΩ ──── 3.3V (Pin 37)
```

### Pines del Pico

| Pin | GPIO | Función | Nota MIDI |
|-----|------|---------|-----------|
| 32 | GP26 | ADC0 | C4 (60) |
| 33 | GP27 | ADC1 | D4 (62) |
| 34 | GP28 | ADC2 | E4 (64) |
| 37 | 3.3V | Alimentación | - |

### Construcción del Touch Pad

1. Cortar lámina de cobre/aluminio (5x5 cm)
2. Soldar cable jumper a la lámina
3. Conectar resistor de 1MΩ entre GPIO y 3.3V
4. Pegar lámina a superficie aislante (cartón, madera)

---

## 💻 Compilación del Firmware

### Primera Vez

```bash
cd /home/joy/src/pico_src/midi
mkdir -p build
cd build
cmake ..
make -j4
```

### Rebuild

```bash
cd /home/joy/src/pico_src/midi/build
make -j4
```

### Build Completo (limpio)

```bash
cd /home/joy/src/pico_src/midi
rm -rf build
mkdir build
cd build
cmake ..
make -j4
```

### Archivo Generado

```
midi/build/src/midi.uf2 (47 KB)
```

---

## 📤 Flashear al Pico

### Método 1: picotool (Recomendado)

```bash
# Poner Pico en modo BOOTSEL
# (Mantener BOOTSEL + conectar USB)

# Flashear
sudo picotool load /home/joy/src/pico_src/midi/build/src/midi.uf2 -f

# Reiniciar
sudo picotool reboot
```

### Método 2: Copia Manual

```bash
# Montar unidad RPI-RP2
sudo mkdir -p /mnt/pico
sudo mount /dev/sda1 /mnt/pico

# Copiar firmware
sudo cp /home/joy/src/pico_src/midi/build/src/midi.uf2 /mnt/pico/

# Desmontar
sync
sudo umount /mnt/pico
```

### Verificar Flasheo

```bash
lsusb | grep cafe
# Debe mostrar: ID cafe:4015 PicoMIDI Pico Touch MIDI
```

---

## 🔊 Audio Bluetooth

### Conectar Speaker

```bash
# Verificar Bluetooth
bluetoothctl info AC:EF:92:D0:B5:BB

# Emparejar (si no está emparejado)
bluetoothctl pair AC:EF:92:D0:B5:BB
bluetoothctl connect AC:EF:92:D0:B5:BB
```

### Iniciar FluidSynth

```bash
python3 /home/joy/src/pico_src/script_tools/start_bt_synth.py
```

### Ajustar Volumen

```bash
# Ver volumen
pactl get-sink-volume bluez_sink.AC_EF_92_D0_B5_BB.a2dp_sink

# Ajustar (0-100%)
pactl set-sink-volume bluez_sink.AC_EF_92_D0_B5_BB.a2dp_sink 50%
```

---

## 🎵 MIDI - Conexiones

### Ver Puertos

```bash
aconnect -l
```

### Conectar Pico a FluidSynth

```bash
aconnect 32:0 128:0
```

### Verificar Conexión

```bash
aconnect -l | grep -A2 'Pico\|FLUID'
```

**Salida esperada:**
```
client 32: 'Pico Touch MIDI'
    0 'Pico Touch MIDI MIDI 1'
    Connecting To: 128:0

client 128: 'FLUID Synth'
    0 'Synth input port'
    Connected From: 32:0
```

---

## 🎹 Cambiar Instrumento

```bash
# Piano (Program 0)
python3 -c "
import mido
port = [p for p in mido.get_output_names() if 'FLUID' in p][0]
with mido.open_output(port) as p:
    p.send(mido.Message('program_change', program=0, channel=0))
"

# Flauta (Program 73)
python3 -c "
import mido
port = [p for p in mido.get_output_names() if 'FLUID' in p][0]
with mido.open_output(port) as p:
    p.send(mido.Message('program_change', program=73, channel=0))
"

# Saxofón (Program 65)
python3 -c "
import mido
port = [p for p in mido.get_output_names() if 'FLUID' in p][0]
with mido.open_output(port) as p:
    p.send(mido.Message('program_change', program=65, channel=0))
"
```

### Instrumentos Populares

| Program | Instrumento |
|---------|-------------|
| 0 | Piano |
| 24 | Guitarra Acústica |
| 25 | Guitarra Steel |
| 40 | Violin |
| 42 | Cello |
| 56 | Trompeta |
| 65 | Saxofón |
| 73 | Flauta |
| 104 | Sitar |
| 107 | Koto |

---

## 🎵 Cambiar Escala

```bash
# Enviar CC#14 al Pico
python3 -c "
import mido
port = [p for p in mido.get_output_names() if 'Pico' in p][0]
with mido.open_output(port) as p:
    # Escala: 0=Major, 1=Minor, 5=Blues, 11=Japanese
    p.send(mido.Message('control_change', control=14, value=5, channel=0))
"
```

### Escalas Disponibles

| # | Escala | Valor CC#14 |
|---|--------|-------------|
| 1 | Major | 0 |
| 2 | Minor | 1 |
| 3 | Harmonic Minor | 2 |
| 4 | Pentatonic Major | 3 |
| 5 | Pentatonic Minor | 4 |
| 6 | Blues | 5 |
| 7 | Dorian | 6 |
| 8 | Mixolydian | 7 |
| 9 | Chromatic | 8 |
| 10 | Whole Tone | 9 |
| 11 | Diminished | 10 |
| 12 | Japanese | 11 |
| 13 | Arabic | 12 |
| 14 | Indian | 13 |

---

## 📊 Monitoreo

### Ver Mensajes MIDI

```bash
python3 /home/joy/src/pico_src/script_tools/test_midi.py
```

### Salida Esperada

```
09:38:59 #  1 🎹 NOTE ON  C4 ( 60) vel:127 ████████████
09:38:59      🎵 NOTE OFF C4 ( 60)
09:39:05 #  2 🎹 NOTE ON  D4 ( 62) vel: 80 ████████
09:39:05      🎵 NOTE OFF D4 ( 62)
```

---

## ⚙️ Configuración del Firmware

### Umbrales de Sensibilidad (main.c)

```c
#define TOUCH_THRESHOLD     3500    // ADC threshold (0-4095)
#define MIN_VELOCITY        20      // Velocity mínima
#define MAX_VELOCITY        127     // Velocity máxima
#define DEBOUNCE_COUNT      2       // Anti-rebote
#define SCAN_INTERVAL_MS    5       // Frecuencia de escaneo
```

### Ajustar Sensibilidad

- **Más sensible:** Aumentar TOUCH_THRESHOLD (hacia 4095)
- **Menos sensible:** Disminuir TOUCH_THRESHOLD (hacia 0)
- **Más rápido:** Disminuir DEBOUNCE_COUNT y SCAN_INTERVAL_MS

---

## 🐛 Troubleshooting

### Pico no aparece en lsusb

```bash
# Verificar USB
lsusb | grep -i "cafe\|2e8a"

# Reiniciar USB
sudo systemctl restart udev
```

### FluidSynth no inicia

```bash
# Matar procesos anteriores
pkill -9 fluidsynth

# Reintentar
python3 /home/joy/src/pico_src/script_tools/start_bt_synth.py &
```

### MIDI no conecta

```bash
# Verificar puertos
aconnect -l

# Reconectar
aconnect 32:0 128:0
```

### No hay sonido Bluetooth

```bash
# Verificar Bluetooth
bluetoothctl info AC:EF:92:D0:B5:BB

# Verificar sink
pactl list sinks short | grep bluetooth

# Reconnect
pactl set-default-sink bluez_sink.AC_EF_92_D0_B5_BB.a2dp_sink
```

### Pads no detectan toque

1. Verificar conexiones (GPIO 26, 27, 28)
2. Verificar resistencias (1MΩ a 3.3V)
3. Ajustar TOUCH_THRESHOLD en main.c
4. Recompilar y flashear

---

## 🔧 Comandos Útiles

### Ver Estado del Sistema

```bash
# USB
lsusb | grep cafe

# MIDI
aconnect -l

# FluidSynth
ps aux | grep fluidsynth

# Bluetooth
pactl list sinks short | grep bluetooth

# Audio
pactl list sink-inputs short
```

### Reiniciar Todo

```bash
# Matar FluidSynth
pkill -9 fluidsynth

# Reiniciar Bluetooth
sudo systemctl restart bluetooth

# Reconectar MIDI
aconnect 32:0 128:0

# Iniciar FluidSynth
python3 /home/joy/src/pico_src/script_tools/start_bt_synth.py &
```

---

## 📚 Aprendizajes Clave

### 1. Sensibilidad ADC

- El ADC del Pico es de 12 bits (0-4095)
- Sin toque: ~4095 (3.3V via resistor)
- Con toque: baja hacia 0 (cuerpo actúa como capacitancia)
- Threshold óptimo: 3000-3500 para touch pads simples

### 2. FluidSynth con stdin

- FluidSynth paniquea si stdin se cierra
- Solución: usar FIFO o pipe que se mantenga abierto
- Python subprocess con stdin_fd es la mejor opción

### 3. Bluetooth Audio

- PulseAudio maneja el routing a Bluetooth
- Usar module-null-sink + module-loopback
- O directamente PULSE_SINK=environment variable

### 4. MIDI Connection

- ALSA MIDI usa clientes y puertos
- Pico = cliente 32, FluidSynth = cliente 128
- aconnect 32:0 128:0 para conectar

### 5. picotool

- Requiere sudo para acceder al USB
- picotool load firmware.uf2 -f para forzar
- picotool reboot -u para reiniciar a BOOTSEL

---

## 🎯 Flujo de Trabajo Completo

```
1. Editar código (main.c)
       ↓
2. Compilar (cmake .. && make -j4)
       ↓
3. Poner Pico en BOOTSEL
       ↓
4. Flashear (sudo picotool load firmware.uf2 -f)
       ↓
5. Verificar (lsusb | grep cafe)
       ↓
6. Conectar MIDI (aconnect 32:0 128:0)
       ↓
7. Iniciar FluidSynth (python3 start_bt_synth.py)
       ↓
8. ¡Tocar los pads!
```

---

## 📞 Referencia Rápida

| Acción | Comando |
|--------|---------|
| Verificar Pico | `lsusb \| grep cafe` |
| Conectar MIDI | `aconnect 32:0 128:0` |
| Iniciar Synth | `python3 start_bt_synth.py` |
| Cambiar Instrumento | `program_change program=NUM` |
| Cambiar Escala | `control_change control=14 value=NUM` |
| Ajustar Volumen | `pactl set-sink-volume BT_SINK NUM%` |
| Compilar | `cmake .. && make -j4` |
| Flashear | `sudo picotool load firmware.uf2 -f` |

---

## 📄 Licencia

MIT License - Libre para uso personal y comercial.

---

## 🔗 Enlaces

- **Repositorio:** https://github.com/siliconvalleyar-oss/pico_src
- **Pico SDK:** https://github.com/raspberrypi/pico-sdk
- **TinyUSB:** https://github.com/hathach/tinyusb
- **FluidSynth:** https://www.fluidsynth.org/
- **Mido:** https://mido.readthedocs.io/
