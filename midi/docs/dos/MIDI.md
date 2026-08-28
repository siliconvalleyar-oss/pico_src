# MIDI - Referencia del Protocolo

## Mensajes MIDI Enviados

### Note On (0x90)
Envocado cuando se toca un pad.

```
Byte 1: 0x90 | canal    (status: Note On, canal 1)
Byte 2: nota MIDI       (60-67)
Byte 3: velocity        (30-127, según presión)
```

**Ejemplo:** Tocar pad 1 (C4)
```
[0x90, 0x3C, 0x7F]  →  Note On canal 1, nota 60 (C4), velocity 127
```

### Note Off (0x80)
Envocado cuando se suelta un pad.

```
Byte 1: 0x80 | canal    (status: Note Off, canal 1)
Byte 2: nota MIDI       (60-67)
Byte 3: 0               (velocity 0)
```

**Ejemplo:** Soltar pad 1
```
[0x80, 0x3C, 0x00]  →  Note Off canal 1, nota 60 (C4)
```

### Control Change (0xB0)
Envocado continuamente mientras se mantiene el toque (valor de presión).

```
Byte 1: 0xB0 | canal    (status: CC, canal 1)
Byte 2: CC number       (nota + 32)
Byte 3: value           (0-127, presión actual)
```

**Ejemplo:** Presión en pad 1
```
[0xB0, 0x5C, 0x40]  →  CC canal 1, CC#92, value 64
```

## Mapeo de Pads a Notas

| Pad | GPIO | Nota MIDI | Nota Musical | Frecuencia |
|-----|------|-----------|--------------|------------|
| 1 | 26 | 60 | C4 | 261.63 Hz |
| 2 | 27 | 62 | D4 | 293.66 Hz |
| 3 | 28 | 64 | E4 | 329.63 Hz |
| 4 | - | 67 | G4 | 392.00 Hz |

## Mapeo de Control Change

| Pad | CC Number | Uso |
|-----|-----------|-----|
| 1 | 92 (60+32) | Presión pad C4 |
| 2 | 94 (62+32) | Presión pad D4 |
| 3 | 96 (64+32) | Presión pad E4 |
| 4 | 99 (67+32) | Presión pad G4 |

## Valor de Velocity

La velocity se calcula del valor ADC:

```
velocity = (TOUCH_THRESHOLD - raw_adc) * 127 / TOUCH_THRESHOLD
```

Donde:
- `TOUCH_THRESHOLD` = 1500 (umbral de detección)
- `raw_adc` = valor leído del ADC (0-4095)
- **Mínimo:** 30 (para que siempre se escuche algo)
- **Máximo:** 127 (toque fuerte)

### Ejemplo de cálculo
```
Raw ADC = 500  → velocity = (1500 - 500) * 127 / 1500 = 84
Raw ADC = 100  → velocity = (1500 - 100) * 127 / 1500 = 118
Raw ADC = 1400 → velocity = (1500 - 1400) * 127 / 1500 = 8 (pero mínimo es 30)
```

## USB MIDI Configuration

- **Clase:** Audio (0x01) / MIDI Streaming (0x03)
- **Vendor ID:** 0xCafe
- **Product ID:** 0x4015
- **Endpoint IN:** 0x81 (64 bytes)
- **Endpoint OUT:** 0x01 (64 bytes)
- **String:** "Pico Touch MIDI"

## Compatibilidad

### DAWs Soportados
- **Ableton Live** - Detecta automáticamente
- **FL Studio** - Detecta automáticamente
- **Logic Pro** - Detecta automáticamente
- **Reaper** - Detecta automáticamente
- **Bitwig Studio** - Detecta automáticamente

### Sintetizadores MIDI
- **Windows:** MIDI-OX, loopMIDI
- **macOS:** SimpleSynth, IAC Driver
- **Linux:** qsynth, qjackctl, FluidSynth

### Frameworks de Desarrollo
- **p5.js** - `navigator.requestMIDIAccess()`
- **Tone.js** - `Tone.Midi`
- **Web MIDI API** - Navegadores modernos

## Cambiar Notas

Para cambiar las notas, editar `main.c`:

```c
// Cambiar este array:
static const uint8_t midi_notes[NUM_PADS] = { 60, 62, 64, 67 };

// Por ejemplo, escala de Do menor:
// static const uint8_t midi_notes[NUM_PADS] = { 60, 63, 65, 67 };

// O escala de Sol mayor:
// static const uint8_t midi_notes[NUM_PADS] = { 55, 57, 59, 62 };
```

## Cambiar Canal MIDI

Para usar un canal diferente, editar la función `midi_task`:

```c
// Cambiar canal de 0 (canal 1) a 1 (canal 2):
send_midi_note_on(1, midi_notes[i], pads[i].velocity);
send_midi_note_off(1, midi_notes[i]);
```

Canales válidos: 0-15 (representan MIDI channels 1-16)
