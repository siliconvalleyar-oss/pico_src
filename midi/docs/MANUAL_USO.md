# Manual de Uso - MIDI Touch Pads

Controlador MIDI USB con touch pads analógicos para Raspberry Pi Pico.

---

## 1. ¿Qué es esto?

Un dispositivo que convierte el toque de tus dedos en sonidos MIDI. Conectas 3 láminas conductoras al Pico, las tocas, y el Pico envía notas musicales a tu computadora por USB como si fuera un teclado MIDI profesional.

---

## 2. Componentes Necesarios

| Componente | Cantidad | Dónde conseguirlo |
|------------|----------|-------------------|
| Raspberry Pi Pico | 1 | Tiendas de electrónica (~$4 USD) |
| Lámina de cobre o aluminio | 3 | Ferretería, craft stores |
| Resistencia 1MΩ | 3 | Tienda de electrónica (~$0.10 c/u) |
| Cable jumper macho-macho | 6 | Tienda de electrónica (~$2 USD) |
| Cable USB micro-B | 1 | El que trae el Pico |

**Total aproximado:** ~$7 USD

---

## 3. Armado del Hardware

### Paso 1: Preparar los touch pads

Corta 3 láminas de cobre/aluminio de aproximadamente 5x5 cm. Si usas cinta adhesiva de aluminio (foil tape), simplemente pégala en una base de cartón o madera.

### Paso 2: Conectar los resistores

Cada touch pad necesita un resistor de 1MΩ conectado entre la lámina y el pin 36 (3.3V) del Pico.

### Paso 3: Conectar los cables

Conecta un cable jumper desde cada lámina hasta el pin correspondiente del Pico:

| Lámina | Pin del Pico | GPIO | Nota MIDI |
|--------|--------------|------|-----------|
| Pad 1 | Pin 4 | GPIO 26 | Do (C4) |
| Pad 2 | Pin 3 | GPIO 27 | Re (D4) |
| Pad 3 | Pin 2 | GPIO 28 | Mi (E4) |

### Paso 4: Verificar conexiones

```
Lámina 1 ──── GPIO 26 (Pin 4) ──── 1MΩ ──── 3.3V (Pin 36)
Lámina 2 ──── GPIO 27 (Pin 3) ──── 1MΩ ──── 3.3V (Pin 36)
Lámina 3 ──── GPIO 28 (Pin 2) ──── 1MΩ ──── 3.3V (Pin 36)
```

**Importante:** Los resistores van de la lámina a 3.3V, NO a GND.

---

## 4. Instalar el Firmware

### Requisitos previos

- Computadora con Windows, macOS o Linux
- Cable USB

### Paso 1: Descargar el firmware

El archivo `midi.uf2` está en:
```
pico_projects/midi/build/src/midi.uf2
```

### Paso 2: Poner el Pico en modo programación

1. **Desconectar** el Pico del USB
2. **Mantener presionado** el botón blanco BOOTSEL
3. **Conectar** el Pico al USB mientras mantienes el botón
4. **Soltar** el botón cuando la luz LED se encienda

Aparece una unidad USB llamada `RPI-RP2` o `RP2040`.

### Paso 3: Copiar el firmware

**Windows:**
```
Copia el archivo midi.uf2 a la unidad RPI-RP2
```

**macOS:**
```bash
cp pico_projects/midi/build/src/midi.uf2 /Volumes/RPI-RP2/
```

**Linux:**
```bash
cp pico_projects/midi/build/src/midi.uf2 /media/$USER/RPI-RP2/
```

### Paso 4: Reiniciar

El Pico se reinicia automáticamente. La LED parpadea indicando que está listo.

---

## 5. Configurar el Sintetizador MIDI

El Pico aparece como dispositivo MIDI USB llamado **"Pico Touch MIDI"**. Necesitas un programa que genere sonido.

### Windows

**Opción 1: MIDI-OX (gratis)**
1. Descargar de https://www.midiox.com/
2. Instalar y abrir
3. Ir a Options → MIDI Devices
4. Seleccionar "Pico Touch MIDI" como entrada
5. El programa genera sonido de prueba

**Opción 2: FL Studio / Ableton / Cualquier DAW**
1. Abrir tu DAW
2. Ir a configuración de MIDI
3. Habilitar "Pico Touch MIDI" como input
4. Asignar un instrumento a una pista
5. ¡Tocar!

### macOS

**Opción 1: GarageBand (gratis)**
1. Abrir GarageBand
2. Crear nueva pista de teclado
3. Ir a Preferencias → Audio/MIDI
4. "Pico Touch MIDI" debe aparecer
5. Tocar en el teclado virtual del Pico

**Opción 2: SimpleSynth (gratis)**
1. Descargar de https://www.notahat.com/simplesynth/
2. Abrir SimpleSynth
3. Seleccionar "Pico Touch MIDI" como input
4. ¡Sonar!

### Linux

**Opción 1: FluidSynth + Qjackctl**
```bash
# Instalar
sudo apt install fluidsynth qsynth qjackctl

# Abrir qsynth, cargar un SoundFont (SF2)
# Abrir qjackctl, conectar Pico Touch MIDI → FluidSynth
```

**Opción 2: VLC (para probar rápido)**
```bash
# VLC puede reproducir archivos MIDI
vlc archivo.mid
# Pero no sirve para MIDI en tiempo real
```

**Opción 3: Timidity**
```bash
sudo apt install timidity
timidity -iA &
# Conectar al puerto MIDI que crea
```

### Web (Navegador)

```html
<!DOCTYPE html>
<html>
<body>
<script>
navigator.requestMIDIAccess().then(midi => {
  midi.inputs.forEach(input => {
    input.onmidimessage = msg => {
      console.log('MIDI:', msg.data);
      // msg.data[0] = status (0x90=Note On, 0x80=Note Off)
      // msg.data[1] = nota (60=C4, 62=D4, 64=E4)
      // msg.data[2] = velocity (0-127)
    };
  });
});
</script>
</body>
</html>
```

---

## 6. Cómo Tocar

### Tocar una nota

Simplemente **toca** una de las láminas con el dedo. El Pico envía un MIDI Note On con la nota correspondiente.

### Controlar la intensidad

- **Toca suave** → sonido bajo (velocity bajo)
- **Toca fuerte** → sonido fuerte (velocity alto)

La velocity va de 30 (mínimo) a 127 (máximo).

### Soltar una nota

Al **levantar** el dedo de la lámina, el Pico envía MIDI Note Off y el sonido para.

### Mantener la nota

Si mantienes el dedo sobre la lámina, el sonido se mantiene. El Pico también envía mensajes de Control Change (CC) que indican la presión actual.

---

## 7. Notas Musicales

| Pad | GPIO | Nota | Frecuencia |
|-----|------|------|------------|
| 1 | 26 | Do (C4) | 261.63 Hz |
| 2 | 27 | Re (D4) | 293.66 Hz |
| 3 | 28 | Mi (E4) | 329.63 Hz |

**Escala de Do Mayor:**
```
Do  Re  Mi  Fa  Sol La  Si  Do
C   D   E   F   G   A   B   C
60  62  64  65  67  69  71  72
```

Con 3 pads tienes las primeras 3 notas de la escala. Puedes cambiar las notas editando el código fuente.

---

## 8. Cambiar las Notas

Para usar notas diferentes, editar el archivo `src/main.c`:

```c
// Notas actuales: Do, Re, Mi
static const uint8_t midi_notes[NUM_PADS] = { 60, 62, 64 };

// Ejemplo: Do, Mi, Sol (acorde de Do Mayor)
static const uint8_t midi_notes[NUM_PADS] = { 60, 64, 67 };

// Ejemplo: Do, Mi bemol, Sol (acorde de Do menor)
static const uint8_t midi_notes[NUM_PADS] = { 60, 63, 67 };

// Ejemplo: Sol, Si, Re (acorde de Sol Mayor)
static const uint8_t midi_notes[NUM_PADS] = { 55, 59, 62 };
```

Después recompilar y flashear de nuevo.

---

## 9. Solución de Problemas

### No suena nada

1. **Verificar conexiones:** ¿Los cables están bien conectados?
2. **Verificar sintetizador:** ¿El programa MIDI está abierto y configurado?
3. **Verificar dispositivo:** ¿Aparece "Pico Touch MIDI" en la lista de dispositivos?
4. **Reiniciar Pico:** Desconectar y reconectar el USB

### Suena sin tocar

1. **Resistencia incorrecta:** Verificar que son 1MΩ, no 1kΩ
2. **Conexión a GND:** Los resistores van a 3.3V, NO a GND
3. **Umbral bajo:** Aumentar `TOUCH_THRESHOLD` en el código

### Suena muy débil o muy fuerte

Ajustar en `src/main.c`:
```c
#define MIN_VELOCITY   30   // Mínimo (bajar para más suave)
#define MAX_VELOCITY   127  // Máximo (subir para más fuerte)
```

### Se desconecta solo

- Usar cable USB de buena calidad
- No usar hubs sin alimentación
- Verificar que no hay cortocircuitos

### No aparece como MIDI

- Reiniciar la computadora
- Probar otro puerto USB
- Probar otro cable USB

---

## 10. Especificaciones Técnicas

| Parámetro | Valor |
|-----------|-------|
| MCU | RP2040 (dual core ARM Cortex-M0+) |
| Frecuencia | 125 MHz |
| ADC | 12 bits (0-4095) |
| Canales ADC | 4 (GPIO 26-29) |
| USB | 2.0 Full Speed (12 Mbps) |
| Clase USB | Audio / MIDI Streaming |
| Vendor ID | 0xCafe |
| Product ID | 0x4015 |
| Nombre USB | Pico Touch MIDI |
| Voltaje de operación | 5V (USB) o 1.8-5.5V (VSYS) |
| Consumo típico | ~50 mA |

---

## 11. Limitaciones

- **3 pads máximo** (por los 3 pines ADC disponibles)
- **Sin polifonía por pad** (un pad = una nota a la vez)
- **Sin Aftertouch** (no detecta presión continua real, solo on/off)
- **Latencia:** ~10ms (ajustable)
- **Sin almacenamiento** (no guarda configuraciones)

---

## 12. Próximas Mejoras Posibles

- [ ] Agregar multiplexor 4051 para 8+ pads
- [ ] Soporte para mod wheel y pitch bend
- [ ] Modo polifónico continuo (CC por presión)
- [ ] LEDs que se encienden al tocar
- [ ] Almacenar configuraciones en flash
- [ ] Interfaz web para configurar notas
- [ ] Soporte para Pico W (Bluetooth MIDI)

---

## 13. Licencia

MIT License - Libre para uso personal y comercial.
