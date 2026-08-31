# HARDWARE - Conexiones y Componentes

## Componentes Necesarios

| Componente | Cantidad | Descripción |
|------------|----------|-------------|
| Raspberry Pi Pico | 1 | RP2040 |
| Módulo KY-037 | 1 | Sensor de detección de sonido |
| Display OLED SSD1306 | 1 | 128x64 pixels, I2C |
| Protoboard | 1 | Para conexiones |
| Cable jumper M-M | 6 | Conexiones |
| Cable USB | 1 | Micro-B para flashear |

---

## Raspberry Pi Pico (RP2040)

### Diagrama de Pines

```
                     RASPBERRY PI PICO
                     ════════════════
                          ┌───────┐
             ┌────────────┤ USB   ├────────────┐
             │            │ Micro │            │
             │            └───────┘            │
             │                                 │
        ┌────┴────┐                      ┌────┴────┐
        │    ●    │   BOOTSEL Button    │    ●    │
        │  RESET  │                      │  LED    │
        └─────────┘                      └─────────┘
             │                                 │
    ┌────────┴─────────────────────────────────┴────────┐
    │ ●  GP0  │ 1 ●                            40 ● │ VSYS   │
    │ ●  GP1  │ 2 ●                            39 ● │ VSYS   │
    │ ●  GP2  │ 3 ●                            38 ● │ 3V3_EN │
    │ ●  GP3  │ 4 ●                            37 ● │ 3V3    │
    │ ●  GP4  │ 5 ●                            36 ● │ ADC_VREF│
    │ ●  GP5  │ 6 ●                            35 ● │ GND    │
    │ ●  GP6  │ 7 ●                            34 ● │ GP28   │
    │ ●  GP7  │ 8 ●                            33 ● │ GP27   │◄── KY-037 AO
    │ ●  GP8  │ 9 ●                            32 ● │ GP26   │◄── KY-037 DO
    │ ●  GP9  │10 ●                            31 ● │ GP22   │
    │ ●  GP10 │11 ●                            30 ● │ GP21   │
    │ ●  GP11 │12 ●                            29 ● │ GP20   │
    │ ●  GP12 │13 ●                            28 ● │ GP19   │
    │ ●  GP13 │14 ●                            27 ● │ GP18   │
    │ ●  GP14 │15 ●                            26 ● │ GP17   │◄── OLED SCL
    │ ●  GP15 │16 ●                            25 ● │ GP16   │◄── OLED SDA
    │ ●  GP16 │17 ●                            24 ● │ GP15   │
    │ ●  GP17 │18 ●                            23 ● │ GP14   │
    │ ●  GP18 │19 ●                            22 ● │ GP13   │
    │ ●  GP19 │20 ●                            21 ● │ GP12   │
    └─────────┴────────────────────────────────────────┘
               │                            │
               │        [Bottom Side]       │
               └────────────────────────────┘

════════════════════════════════════════════════════
             Pin Labels (Top View)
════════════════════════════════════════════════════

LEFT SIDE                          RIGHT SIDE
─────────                          ───────────
Pin  1: GP0  (UART0 TX)           Pin 40: VSYS (Voltage System)
Pin  2: GP1  (UART0 RX)           Pin 39: VSYS
Pin  3: GP2  (I2C1 SDA)           Pin 38: 3V3_EN (3.3V Enable)
Pin  4: GP3  (I2C1 SCL)           Pin 37: 3V3 (3.3V Output)
Pin  5: GP4  (I2C0 SDA)           Pin 36: ADC_VREF (ADC Reference)
Pin  6: GP5  (I2C0 SCL)           Pin 35: GND
Pin  7: GP6                       Pin 34: GP28 (ADC2)
Pin  8: GP7                       Pin 33: GP27 (ADC1) ◄── KY-037 AO
Pin  9: GP8                       Pin 32: GP26 (ADC0) ◄── KY-037 DO
Pin 10: GP9                       Pin 31: GP22
Pin 11: GP10                      Pin 30: GP21
Pin 12: GP11                      Pin 29: GP20
Pin 13: GP12                      Pin 28: GP19
Pin 14: GP13                      Pin 27: GP18
Pin 15: GP14                      Pin 26: GP17 ◄── OLED SCL
Pin 16: GP15                      Pin 25: GP16 ◄── OLED SDA
Pin 17: GP16                      Pin 24: GP15
Pin 18: GP17                      Pin 23: GP14
Pin 19: GP18                      Pin 22: GP13
Pin 20: GP19                      Pin 21: GP12

════════════════════════════════════════════════════
             Especificaciones
════════════════════════════════════════════════════

• MCU: RP2040 (Dual Core ARM Cortex-M0+)
• Flash: 2MB
• RAM: 264KB SRAM
• USB: Micro-B 2.0
• ADC: 4 canales (GPIO 26-29)
• I2C: 2 controladores (I2C0, I2C1)
• Dimensiones: 51mm x 21mm
• Alimentación: 1.8V - 5.5V (VSYS)
```

---

## Conexiones del Potenciómetro

### Potenciómetro → GPIO 26 (ADC0)

```
     ┌─────────────┐
     │   POT 10k   │
     │             │
  GND├────●        ├──●──── GPIO 26 (Pin 32)
        │        │
        │        │
      OUT ●      3.3V (Pin 37)
              │
             3.3V
```

| Pin Potenciómetro | Pin Pico | GPIO | Función |
|-------------------|----------|------|---------|
| Izquierdo | Pin 32 | GP26 | Entrada ADC |
| Central | - | - | Salida analógica |
| Derecho | Pin 37 | 3V3 | Alimentación 3.3V |

---

## Conexiones del KY-037

### KY-037 → GPIO 26 y 27

```
     ┌──────────────────┐
     │   KY-037 Module  │
     │                  │
  VCC ├──────────────────┤ 3V3 (Pin 37)
  GND ├──────────────────┤ GND (Pin 35)
  AO  ├──────────────────┤ GP27 (Pin 33) ◄── Analog
  DO  ├──────────────────┤ GP26 (Pin 32) ◄── Digital
     └──────────────────┘
```

| Pin KY-037 | Pin Pico | GPIO | Función |
|-------------|----------|------|---------|
| VCC | Pin 37 | 3V3 | Alimentación 3.3V |
| GND | Pin 35 | GND | Tierra |
| AO | Pin 33 | GP27 | Salida analógica (ADC1) |
| DO | Pin 32 | GP26 | Salida digital |

**Nota:** El KY-037 tiene un potenciómetro integrado para ajustar la sensibilidad del umbral digital. Gíralo para ajustar qué tan sensible es la detección de sonido.

---

## Conexiones del OLED SSD1306

### OLED → I2C0 (GPIO 16 y 17)

```
     ┌──────────────────┐
     │   SSD1306 OLED   │
     │   128x64 I2C     │
     │                  │
  GND ├──────────────────┤ GND (Pin 35)
  VCC ├──────────────────┤ 3V3 (Pin 37)
  SDA ├──────────────────┤ GP16 (Pin 25)
  SCL ├──────────────────┤ GP17 (Pin 26)
     └──────────────────┘
```

| Pin OLED | Pin Pico | GPIO | Función |
|----------|----------|------|---------|
| GND | Pin 35 | GND | Tierra |
| VCC | Pin 37 | 3V3 | Alimentación 3.3V |
| SDA | Pin 25 | GP16 | Datos I2C0 |
| SCL | Pin 26 | GP17 | Reloj I2C0 |

---

## Conexión USB (CDC Serial)

Además de las conexiones analógicas y I2C, el Pico se conecta por USB para:

| Función | Puerto USB | Uso |
|---------|-----------|-----|
| Alimentación | USB Micro-B | 5V desde PC/USB |
| Datos CDC | USB Micro-B | Monitor serial de debug |

**Conexión USB:**
- Conecta el Pico a la computadora por USB Micro-B
- El Pico aparecerá como dispositivo **USB CDC** (Virtual Serial Port)
- En Linux: `/dev/ttyACM0`
- En Windows: `COMx`
- En macOS: `/dev/cu.usbmodem...`

**Datos que se envían por USB CDC (cada 500ms):**
```
[DATA] ADC= 1234 | V= 1.23V | DO=LOW  | OLED=OK
```

---

## Esquema General

```
    ┌──────────────────────────────────────────────────────────┐
    │                    RASPBERRY PI PICO                     │
    │                                                          │
    │    Pin 37: 3V3 ────────┬──── VCC (OLED + KY-037)        │
    │    Pin 35: GND ────────┼──── GND (OLED + KY-037)        │
    │                         │                                │
    │    Pin 33: GP27 (ADC1) ──── AO (KY-037)                 │
    │    Pin 32: GP26       ──── DO (KY-037)                  │
    │                         │                                │
    │    Pin 25: GP16 (I2C0 SDA) ──── SDA (OLED)              │
    │    Pin 26: GP17 (I2C0 SCL) ──── SCL (OLED)              │
    │                         │                                │
    │    USB Micro-B ─────────┴──── Datos CDC Serial          │
    └──────────────────────────────────────────────────────────┘
                           │
                           │ USB
                           ▼
                       PC / USB
```

---

## Conexión Alternativa: I2C1 (GPIO 2 y 3)

Si prefieres usar I2C1 en lugar de I2C0, conecta:

| Pin OLED | Pin Pico | GPIO | Función |
|----------|----------|------|---------|
| SDA | Pin 3 | GP2 | Datos I2C1 |
| SCL | Pin 4 | GP3 | Reloj I2C1 |

**Nota:** Debes modificar el código fuente (`main.c`) para usar `i2c1` en lugar de `i2c0` y cambiar los pines GPIO 16 y 17.

---

## Notas Importantes

- **GPIO 26, 27, 28, 29** son los únicos pines con ADC en el RP2040
- **I2C0 e I2C1** son los dos controladores I2C disponibles
- El OLED SSD1306 funciona a 3.3V, no requiere level shifter
- La dirección I2C del SSD1306 suele ser **0x3C** o **0x3D** (depende del modelo)
- Si el OLED no funciona, verifica la dirección I2C con un scanner
- Usa cables cortos para evitar ruido en las señales I2C y ADC
- El KY-037 funciona con 3.3V o 5V, pero con 3.3V la salida digital es compatible con el Pico
