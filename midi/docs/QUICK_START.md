# 🚀 Guía Rápida - MIDI Touch Pads

Comandos prácticos para usar el sistema MIDI completo.

---

## 📋 Comandos Esenciales

### Verificar Estado del Sistema

```bash
# Verificar dispositivo MIDI
ssh joy@raspberry.local "lsusb | grep cafe"

# Verificar conexiones ALSA
ssh joy@raspberry.local "aconnect -l"

# Verificar FluidSynth
ssh joy@raspberry.local "ps aux | grep fluidsynth"

# Verificar Bluetooth
ssh joy@raspberry.local "pactl list sinks short | grep bluetooth"
```

### Iniciar Sistema MIDI

```bash
# Iniciar FluidSynth con Bluetooth
ssh joy@raspberry.local "python3 /home/joy/src/pico_src/script_tools/start_bt_synth.py &"

# Conectar MIDI
ssh joy@raspberry.local "aconnect 32:0 128:0"
```

### Monitorear Pads

```bash
# Ver toques en tiempo real
ssh joy@raspberry.local "python3 /home/joy/src/pico_src/script_tools/test_midi.py"
```

---

## 🎹 Cambiar Instrumento

```bash
# Piano (Program 0)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'FLUID' in x][0]); p.send(mido.Message('program_change', program=0, channel=0))\""

# Guitarra Acústica (Program 24)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'FLUID' in x][0]); p.send(mido.Message('program_change', program=24, channel=0))\""

# Violin (Program 40)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'FLUID' in x][0]); p.send(mido.Message('program_change', program=40, channel=0))\""

# Saxofón (Program 65)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'FLUID' in x][0]); p.send(mido.Message('program_change', program=65, channel=0))\""

# Flauta (Program 73)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'FLUID' in x][0]); p.send(mido.Message('program_change', program=73, channel=0))\""
```

---

## 🎵 Cambiar Escala

```bash
# Major (0)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'Pico' in x][0]); p.send(mido.Message('control_change', control=14, value=0, channel=0))\""

# Minor (1)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'Pico' in x][0]); p.send(mido.Message('control_change', control=14, value=1, channel=0))\""

# Pentatonic Minor (4)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'Pico' in x][0]); p.send(mido.Message('control_change', control=14, value=4, channel=0))\""

# Blues (5)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'Pico' in x][0]); p.send(mido.Message('control_change', control=14, value=5, channel=0))\""

# Japanese (11)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'Pico' in x][0]); p.send(mido.Message('control_change', control=14, value=11, channel=0))\""

# Arabic (12)
ssh joy@raspberry.local "python3 -c \"import mido; p=mido.open_output([x for x in mido.get_output_names() if 'Pico' in x][0]); p.send(mido.Message('control_change', control=14, value=12, channel=0))\""
```

---

## 🔊 Ajustar Volumen Bluetooth

```bash
# Ver volumen actual
ssh joy@raspberry.local "pactl get-sink-volume bluez_sink.AC_EF_92_D0_B5_BB.a2dp_sink"

# Ajustar volumen (0-100%)
ssh joy@raspberry.local "pactl set-sink-volume bluez_sink.AC_EF_92_D0_B5_BB.a2dp_sink 50%"

# Silenciar
ssh joy@raspberry.local "pactl set-sink-mute bluez_sink.AC_EF_92_D0_B5_BB.a2dp_sink 1"

# Activar sonido
ssh joy@raspberry.local "pactl set-sink-mute bluez_sink.AC_EF_92_D0_B5_BB.a2dp_sink 0"
```

---

## 🔧 Compilar y Flashear Firmware

```bash
# Compilar y flashear
ssh joy@raspberry.local "bash /home/joy/src/pico_src/script_tools/flash_full.sh"

# Solo compilar
ssh joy@raspberry.local "bash /home/joy/src/pico_src/script_tools/flash_full.sh --compile-only"

# Solo flashear
ssh joy@raspberry.local "bash /home/joy/src/pico_src/script_tools/flash_full.sh --flash-only"
```

---

## 🛠️ Solución de Problemas

### FluidSynth no inicia
```bash
# Matar procesos anteriores
ssh joy@raspberry.local "pkill -9 fluidsynth"

# Reintentar
ssh joy@raspberry.local "python3 /home/joy/src/pico_src/script_tools/start_bt_synth.py &"
```

### MIDI no conecta
```bash
# Verificar puertos
ssh joy@raspberry.local "aconnect -l"

# Reconectar
ssh joy@raspberry.local "aconnect 32:0 128:0"
```

### No hay sonido Bluetooth
```bash
# Verificar Bluetooth
ssh joy@raspberry.local "bluetoothctl info AC:EF:92:D0:B5:BB"

# Verificar sink
ssh joy@raspberry.local "pactl list sinks short | grep bluetooth"
```

### Pico no detectado
```bash
# Verificar USB
ssh joy@raspberry.local "lsusb | grep -i 'cafe\|2e8a'"

# Reiniciar USB
ssh joy@raspberry.local "sudo systemctl restart udev"
```

---

## 📊 Referencia Rápida

### Notas MIDI
| Nota | MIDI | Frecuencia |
|------|------|------------|
| C4 | 60 | 261.63 Hz |
| D4 | 62 | 293.66 Hz |
| E4 | 64 | 329.63 Hz |
| F4 | 65 | 349.23 Hz |
| G4 | 67 | 392.00 Hz |
| A4 | 69 | 440.00 Hz |
| B4 | 71 | 493.88 Hz |

### Escalas
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

### Instrumentos Populares
| Instrumento | Program |
|-------------|---------|
| Piano | 0 |
| Guitarra Acústica | 24 |
| Guitarra Eléctrica | 27 |
| Violin | 40 |
| Cello | 42 |
| Trompeta | 56 |
| Saxofón | 65 |
| Flauta | 73 |
| Órgano | 19 |
| Sitar | 104 |
| Koto | 107 |

---

## 📁 Scripts Disponibles

| Script | Uso |
|--------|-----|
| `check_midi.sh` | Verificar estado MIDI |
| `test_midi.py` | Monitorear toques |
| `flash_full.sh` | Compilar y flashear |
| `start_bt_synth.py` | Iniciar synth Bluetooth |
| `install_midi_tools.sh` | Instalar dependencias |
| `change_instrument.sh` | Cambiar instrumento |

---

## 🔗 Enlaces

- **Repositorio:** https://github.com/siliconvalleyar-oss/pico_src
- **Documentación:** [README.md](../../README.md)
- **Manual de Usuario:** [MANUAL_USO.md](MANUAL_USO.md)
- **Escalas Musicales:** [SCALES.md](SCALES.md)
