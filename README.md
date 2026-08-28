# 🎹 Pico Source - Raspberry Pi Pico Projects

Proyectos para Raspberry Pi Pico con foco en audio MIDI USB.

## 📋 Tabla de Contenidos

- [Descripción](#descripción)
- [Estructura del Proyecto](#estructura-del-proyecto)
- [Instalación Rápida](#instalación-rápida)
- [Proyecto MIDI Touch Pads](#midi-touch-pads)
- [Scripts Disponibles](#scripts-disponibles)
- [Documentación](#documentación)
- [Troubleshooting](#troubleshooting)
- [Licencia](#licencia)

---

## 📝 Descripción

**Pico Source** es un conjunto de proyectos para Raspberry Pi Pico enfocados en crear controladores MIDI USB. El proyecto principal convierte touch pads analógicos en un instrumento musical MIDI.

### Características Principales

- ✅ USB MIDI nativo (TinyUSB)
- ✅ 3 touch pads con detección de presión (velocity)
- ✅ 14 escalas musicales diferentes
- ✅ Cambio de escala por botón o MIDI CC
- ✅ Compilación remota via SSH
- ✅ Documentación completa en español

---

## 📁 Estructura del Proyecto

```
pico_src/
│
├── README.md                          # Este archivo
├── VERSION                            # Versión del proyecto
├── .gitignore                         # Archivos ignorados por git
│
├── midi/                              # 🎹 Proyecto MIDI Touch Pads
│   ├── CMakeLists.txt                 # Configuración CMake principal
│   ├── pico_sdk_import.cmake          # Importador del SDK
│   ├── .gitignore                     # Gitignore del proyecto
│   │
│   ├── src/                           # Código fuente
│   │   ├── CMakeLists.txt             # Build del firmware
│   │   ├── main.c                     # Código principal (ADC + MIDI)
│   │   ├── tusb_config.h              # Configuración TinyUSB
│   │   ├── usb_descriptors.c          # Descriptores USB MIDI
│   │   ├── usb_descriptors.h          # Header de descriptores
│   │   └── README.md                  # Readme del código
│   │
│   └── docs/                          # 📚 Documentación
│       ├── README.md                  # Índice de documentación
│       ├── MANUAL_USO.md              # Manual de usuario completo
│       ├── HARDWARE.md                # Conexiones y hardware
│       ├── BUILD.md                   # Guía de compilación
│       ├── MIDI.md                    # Referencia MIDI
│       ├── SCALES.md                  # Escalas musicales
│       ├── REMOTE_FLASH.md            # Flasheo remoto
│       ├── TROUBLESHOOTING.md         # Solución de problemas
│       ├── pico_1.jpeg                # Imagen Pico 1
│       ├── pico2w-pinout.svg          # Pinout Pico 2W
│       ├── pico.jpeg                  # Imagen Pico
│       ├── raspbery_pico.png          # Diagrama Pico
│       └── raspbery_pico_1.png        # Diagrama Pico 1
│
├── script_tools/                      # 🔧 Scripts de utilidad
│   ├── install_deps.sh                # Instalar dependencias
│   ├── fix_first_build.sh             # Corregir primer build
│   ├── quick_rebuild.sh               # Rebuild rápido
│   ├── flash.sh                       # Flashear firmware
│   ├── flash_full.sh                  # Compile + flash completo
│   ├── check_midi.sh                  # Verificar dispositivo MIDI
│   ├── start_synth.sh                 # Iniciar FluidSynth
│   ├── test_midi.py                   # Test MIDI en consola
│   └── run_synth.py                   # Ejecutar synth
│
└── .agents/                           # 🤖 Skills
    └── skills/
        └── pico-remote-flash/
            └── SKILL.md               # Skill de flasheo remoto
```

---

## 🚀 Instalación Rápida

### En la Raspberry Pi

```bash
# 1. Clonar el repositorio
git clone https://github.com/siliconvalleyar-oss/pico_src.git
cd pico_src

# 2. Instalar dependencias
sudo bash script_tools/install_deps.sh

# 3. Primera compilación
bash script_tools/fix_first_build.sh

# 4. Flashear al Pico
bash script_tools/flash_full.sh
```

### Requisitos

| Componente | Versión Mínima |
|------------|----------------|
| Raspberry Pi | 3B+ o superior |
| Raspberry Pi Pico | 1 o 2W |
| GCC ARM | 12.x |
| CMake | 3.13+ |
| Python | 3.7+ |
| Pico SDK | 2.x |

---

## 🎹 MIDI Touch Pads

### ¿Qué es?

Un controlador MIDI USB que convierte el toque de tus dedos en sonidos musicales. Conectas 3 láminas conductoras al Pico, las tocas, y el Pico envía notas musicales a tu computadora.

### Hardware Necesario

| Componente | Cantidad | Costo Aprox. |
|------------|----------|--------------|
| Raspberry Pi Pico | 1 | ~$4 USD |
| Lámina de cobre/aluminio | 3 | ~$1 USD |
| Resistencia 1MΩ | 3 | ~$0.30 USD |
| Cable jumper M-M | 6 | ~$2 USD |
| USB cable | 1 | Incluido con Pico |

**Total:** ~$7 USD

### Conexiones

```
Lámina 1 ──── GPIO 26 (Pin 32) ──── 1MΩ ──── 3.3V (Pin 37)
Lámina 2 ──── GPIO 27 (Pin 33) ──── 1MΩ ──── 3.3V (Pin 37)
Lámina 3 ──── GPIO 28 (Pin 34) ──── 1MΩ ──── 3.3V (Pin 37)
```

### Notas por Defecto

| Pad | GPIO | Nota | Frecuencia |
|-----|------|------|------------|
| 1 | 26 | C4 (Do) | 261.63 Hz |
| 2 | 27 | D4 (Re) | 293.66 Hz |
| 3 | 28 | E4 (Mi) | 329.63 Hz |

### Escalas Disponibles (14)

| # | Escala | Carácter |
|---|--------|----------|
| 1 | Major | Alegre, brillante |
| 2 | Minor | Triste, melancólico |
| 3 | Harmonic Minor | Exótico, dramático |
| 4 | Pentatonic Major | Universal |
| 5 | Pentatonic Minor | Blues, rock |
| 6 | Blues | Blues, jazz |
| 7 | Dorian | Jazz, funk |
| 8 | Mixolydian | Rock, celta |
| 9 | Chromatic | Experimental |
| 10 | Whole Tone | Onírico |
| 11 | Diminished | Suspenso |
| 12 | Japanese | Tradicional |
| 13 | Arabic | Medio Oriente |
| 14 | Indian | Clásico indio |

### Cambiar de Escala

**Método 1: Botón (GPIO 15)**
```
GPIO 15 ──── Botón ──── GND
```

**Método 2: MIDI CC#14**
```python
port.send(mido.Message('control_change', control=14, value=5))
```

---

## 🔧 Scripts Disponibles

### Para Primera Vez

| Script | Uso | Descripción |
|--------|-----|-------------|
| `install_deps.sh` | `sudo bash install_deps.sh` | Instala todas las dependencias |
| `fix_first_build.sh` | `bash fix_first_build.sh` | Corrige problemas comunes |

### Para Desarrollo

| Script | Uso | Descripción |
|--------|-----|-------------|
| `quick_rebuild.sh` | `bash quick_rebuild.sh` | Rebuild rápido |
| `quick_rebuild.sh --clean` | Limpieza completa | Rebuild limpio |

### Para Flasheo

| Script | Uso | Descripción |
|--------|-----|-------------|
| `flash_full.sh` | `bash flash_full.sh` | Compile + flash completo |
| `flash.sh` | `bash flash.sh` | Solo flash |

### Para Testing

| Script | Uso | Descripción |
|--------|-----|-------------|
| `check_midi.sh` | `bash check_midi.sh` | Verificar dispositivo MIDI |
| `test_midi.py` | `python3 test_midi.py` | Ver mensajes MIDI en consola |

### Para Audio

| Script | Uso | Descripción |
|--------|-----|-------------|
| `start_synth.sh` | `bash start_synth.sh` | Iniciar FluidSynth |
| `run_synth.py` | `python3 run_synth.py` | Ejecutar synth |

---

## 📚 Documentación

### Archivos Principales

| Archivo | Contenido |
|---------|-----------|
| [MANUAL_USO.md](midi/docs/MANUAL_USO.md) | Manual completo de usuario |
| [HARDWARE.md](midi/docs/HARDWARE.md) | Diagramas de hardware y conexiones |
| [BUILD.md](midi/docs/BUILD.md) | Guía de compilación y flasheo |
| [MIDI.md](midi/docs/MIDI.md) | Referencia del protocolo MIDI |
| [SCALES.md](midi/docs/SCALES.md) | Escalas musicales disponibles |
| [REMOTE_FLASH.md](midi/docs/REMOTE_FLASH.md) | Flasheo remoto via SSH |
| [TROUBLESHOOTING.md](midi/docs/TROUBLESHOOTING.md) | Solución de problemas |

### Diagramas

| Archivo | Contenido |
|---------|-----------|
| [pico_1.jpeg](midi/docs/pico_1.jpeg) | Raspberry Pi Pico 1 |
| [pico2w-pinout.svg](midi/docs/pico2w-pinout.svg) | Pinout Pico 2W |
| [raspbery_pico.png](midi/docs/raspbery_pico.png) | Diagrama de conexión |

---

## 🔌 Uso con Sintetizadores

### Linux (Raspberry Pi)

```bash
# Instalar FluidSynth
sudo apt install fluidsynth fluid-soundfont-gm

# Iniciar synth
bash script_tools/start_synth.sh

# O manualmente
fluidsynth -a alsa -m alsa_seq /usr/share/sounds/sf2/FluidR3_GM.sf2 &
aconnect 32:0 128:0
```

### Windows

- MIDI-OX: https://www.midiox.com/
- FL Studio, Ableton, Reaper

### macOS

- SimpleSynth: https://www.notahat.com/simplesynth/
- GarageBand (incluido)

### Web (JavaScript)

```javascript
navigator.requestMIDIAccess().then(midi => {
    midi.inputs.forEach(input => {
        input.onmidimessage = msg => {
            console.log('MIDI:', msg.data);
        };
    });
});
```

---

## 🐛 Troubleshooting

### El Pico no aparece en `lsusb`

```bash
# Verificar conexión USB
lsusb | grep -i "cafe\|2e8a"

# Si no aparece, reiniciar USB
sudo systemctl restart udev
```

### Error de compilación

```bash
# Verificar dependencias
arm-none-eabi-gcc --version
cmake --version

# Reinstalar si es necesario
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential cmake
```

### No hay sonido

```bash
# Verificar conexiones MIDI
aconnect -l

# Conectar Pico a FluidSynth
aconnect 32:0 128:0

# Verificar volumen
alsamixer
```

### Ver logs de debug

```bash
# Habilitar stdio en main.c:
# stdio_init_all();

# Ver por serial:
screen /dev/ttyACM0 115200
```

---

## 📊 Especificaciones Técnicas

| Parámetro | Valor |
|-----------|-------|
| MCU | RP2040 (Dual Core Cortex-M0+) |
| Frecuencia | 125 MHz |
| Flash | 2MB |
| RAM | 264KB |
| USB | 2.0 Full Speed (12 Mbps) |
| Clase USB | Audio / MIDI Streaming |
| Vendor ID | 0xCafe |
| Product ID | 0x4015 |
| Nombre USB | Pico Touch MIDI |
| ADC | 12 bits (0-4095) |
| Canales ADC | 4 (GPIO 26-29) |
| Voltaje | 1.8V - 5.5V |
| Consumo | ~50 mA típico |

---

## 🤝 Contribuir

1. Fork el repositorio
2. Crear branch para nueva feature (`git checkout -b feature/nueva-escala`)
3. Commit cambios (`git commit -m 'Add nueva escala'`)
4. Push al branch (`git push origin feature/nueva-escala`)
5. Abrir Pull Request

---

## 📄 Licencia

MIT License - Ver [LICENSE](LICENSE) para detalles.

---

## 🔗 Enlaces

- **Repositorio:** https://github.com/siliconvalleyar-oss/pico_src
- **Pico SDK:** https://github.com/raspberrypi/pico-sdk
- **TinyUSB:** https://github.com/hathach/tinyusb
- **FluidSynth:** https://www.fluidsynth.org/

---

## 🙏 Agradecimientos

- Raspberry Pi Foundation por el SDK
- Ha Thach por TinyUSB
- Comunidad de open source
