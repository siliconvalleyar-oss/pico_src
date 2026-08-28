# Escalas Musicales Disponibles

El MIDI Touch Pads soporta 14 escalas musicales diferentes.

## Escalas Incluidas

| # | Escala | Notas | Ejemplo (C4) | Carácter |
|---|--------|-------|--------------|----------|
| 1 | **Major** | 7 | C D E F G A B | Alegre, brillante |
| 2 | **Minor** | 7 | C D Eb F G Ab Bb | Triste, melancólico |
| 3 | **Harmonic Minor** | 7 | C D Eb F G Ab B | Exótico, dramático |
| 4 | **Pentatonic Major** | 5 | C D E G A | Universal, consonante |
| 5 | **Pentatonic Minor** | 5 | C Eb F G Bb | Blues, rock, funk |
| 6 | **Blues** | 6 | C Eb F Gb G Bb | Blues, jazz, soul |
| 7 | **Dorian** | 7 | C D Eb F G A Bb | Jazz, funk, modal |
| 8 | **Mixolydian** | 7 | C D E F G A Bb | Rock, blues, celta |
| 9 | **Chromatic** | 12 | Todos los semitonos | Experimental |
| 10 | **Whole Tone** | 6 | C D E F# G# A# | Onírico, impresionista |
| 11 | **Diminished** | 8 | Alternando tonos | Tensión, suspenso |
| 12 | **Japanese** | 5 | C Db F G Ab | tradicional japonés |
| 13 | **Arabic** | 7 | C Db E F G Ab B | Medio Oriente |
| 14 | **Indian** | 7 | C Db E F G Ab B | Clásico indio |

## Cómo Cambiar de Escala

### Método 1: Botón Físico
Conectar un botón entre **GPIO 15** y **GND**. Cada pulso cambia a la siguiente escala.

```
GPIO 15 ──── Botón ──── GND
```

La LED del Pico parpadeará indicando la escala actual:
- 1 flash = Major
- 2 flashes = Minor
- 3 flashes = Harmonic Minor
- etc.

### Método 2: Mensaje MIDI (CC)
Enviar un mensaje Control Change desde otra aplicación:

| CC# | Función | Rango |
|-----|---------|-------|
| 14 | Cambiar escala | 0-127 (mod 14) |
| 15 | Cambiar octava | 0-127 (mapeado a -3 a +3) |

**Ejemplo en Python:**
```python
import mido

with mido.open_output('Pico Touch MIDI') as port:
    # Cambiar a escala #5 (Pentatonic Minor)
    port.send(mido.Message('control_change', control=14, value=5))
    
    # Subir una octava
    port.send(mido.Message('control_change', control=15, value=85))
```

### Método 3: Editar el Código
Cambiar las variables al inicio de `main.c`:

```c
// Escala inicial (0-13)
static uint8_t current_scale = 0;  // 0=Major, 1=Minor, etc.

// Nota raíz (MIDI note)
static uint8_t root_note = 60;  // C4

// Offset de octava
static int8_t octave_offset = 0;  // -3 a +3
```

## Notas Raíz Comunes

| Nota | MIDI | Frecuencia |
|------|------|------------|
| C4 | 60 | 261.63 Hz |
| D4 | 62 | 293.66 Hz |
| E4 | 64 | 329.63 Hz |
| F4 | 65 | 349.23 Hz |
| G4 | 67 | 392.00 Hz |
| A4 | 69 | 440.00 Hz |
| B4 | 71 | 493.88 Hz |

## Mapeo de Pads a Notas

### Major Scale (C4)
```
Pad 1: C4 (60)  ── Do
Pad 2: D4 (62)  ── Re
Pad 3: E4 (64)  ── Mi
```

### Minor Scale (C4)
```
Pad 1: C4  (60) ── Do
Pad 2: D4  (62) ── Re
Pad 3: Eb4 (63) ── Mi bemol
```

### Pentatonic Minor (C4)
```
Pad 1: C4  (60) ── Do
Pad 2: Eb4 (63) ── Mi bemol
Pad 3: F4  (65) ── Fa
```

### Blues (C4)
```
Pad 1: C4  (60) ── Do
Pad 2: Eb4 (63) ── Mi bemol
Pad 3: F4  (65) ── Fa
```

### Japanese (C4)
```
Pad 1: C4  (60) ── Do
Pad 2: Db4 (61) ── Do sostenido
Pad 3: F4  (65) ── Fa
```

## Teoría Rápida

### ¿Por qué 3 pads?

Con solo 3 notas puedes tocar:
- **Triadas** (acordes de 3 notas)
- **Fragmentos de melodías**
- **Bases rítmicas**

### Pentatónica es universal

La escala pentatónica funciona en几乎 cualquier contexto musical:
- Rock, blues, jazz, pop, country, música folclórica
- Es la escala más usada en la música mundial

### Modos griegos

Los modos (Dorian, Mixolydian, etc.) son como escalas mayores "desplazadas":
- Dorian = Major desde el 2do grado
- Mixolydian = Major desde el 5to grado

## Ejercicios

1. **Major → Minor**: Toca C-D-E en Major, luego cambia a Minor y nota la diferencia
2. **Blues**: Cambia a Blues y toca sobre un backing track de blues
3. **Japanese**: Toca la escala japonesa para sonidos exóticos
4. **Modula**: Usa CC#14 para cambiar de escala mientras tocas
