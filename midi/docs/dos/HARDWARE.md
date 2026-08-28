# HARDWARE - Conexiones y Componentes

## Componentes Necesarios

| Componente | Cantidad | Descripción |
|------------|----------|-------------|
| Raspberry Pi Pico | 1 | Pico 1 (RP2040) o Pico 2W (RP2350) |
| Lámina de cobre/aluminio | 3 | Touch pads (5x5 cm mínimo) |
| Resistencia 1MΩ | 3 | Pull-up para cada pad |
| Cable jumper M-M | 6 | Conexiones |
| USB cable | 1 | Micro-B (Pico 1) o USB-C (Pico 2W) |

---

## Raspberry Pi Pico 1 (RP2040)

### Diagrama de Pines

```
                    RASPBERRY PI PICO 1
                    ═══════════════════
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
   │ ●  GP7  │ 8 ●                            33 ● │ GP27   │
   │ ●  GP8  │ 9 ●                            32 ● │ GP26   │
   │ ●  GP9  │10 ●                            31 ● │ GP22   │
   │ ●  GP10 │11 ●                            30 ● │ GP21   │
   │ ●  GP11 │12 ●                            29 ● │ GP20   │
   │ ●  GP12 │13 ●                            28 ● │ GP19   │
   │ ●  GP13 │14 ●                            27 ● │ GP18   │
   │ ●  GP14 │15 ●                            26 ● │ GP17   │
   │ ●  GP15 │16 ●                            25 ● │ GP16   │
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
   ─────────                          ──────────
   Pin  1: GP0  (UART0 TX)           Pin 40: VSYS (Voltage System)
   Pin  2: GP1  (UART0 RX)           Pin 39: VSYS
   Pin  3: GP2  (UART0 CTS)          Pin 38: 3V3_EN (3.3V Enable)
   Pin  4: GP3  (UART0 RTS)          Pin 37: 3V3 (3.3V Output)
   Pin  5: GP4  (I2C0 SDA)           Pin 36: ADC_VREF (ADC Reference)
   Pin  6: GP5  (I2C0 SCL)           Pin 35: GND
   Pin  7: GP6                       Pin 34: GP28 (ADC2) ◄── TOUCH 3
   Pin  8: GP7                       Pin 33: GP27 (ADC1) ◄── TOUCH 2
   Pin  9: GP8                       Pin 32: GP26 (ADC0) ◄── TOUCH 1
   Pin 10: GP9                       Pin 31: GP22
   Pin 11: GP10                      Pin 30: GP21
   Pin 12: GP11                      Pin 29: GP20
   Pin 13: GP12                      Pin 28: GP19
   Pin 14: GP13                      Pin 27: GP18
   Pin 15: GP14                      Pin 26: GP17
   Pin 16: GP15                      Pin 25: GP16
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
   • Dimensiones: 51mm × 21mm
   • Alimentación: 1.8V - 5.5V (VSYS)
   • Consumo: ~50mA típico
```

---

## Raspberry Pi Pico 2W (RP2350)

### Diagrama de Pines

```
                    RASPBERRY PI PICO 2W
                    ════════════════════
                         ┌───────┐
            ┌────────────┤ USB   ├────────────┐
            │            │ USB-C │            │
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
   │ ●  GP7  │ 8 ●                            33 ● │ GP27   │
   │ ●  GP8  │ 9 ●                            32 ● │ GP26   │
   │ ●  GP9  │10 ●                            31 ● │ GP22   │
   │ ●  GP10 │11 ●                            30 ● │ GP21   │
   │ ●  GP11 │12 ●                            29 ● │ GP20   │
   │ ●  GP12 │13 ●                            28 ● │ GP19   │
   │ ●  GP13 │14 ●                            27 ● │ GP18   │
   │ ●  GP14 │15 ●                            26 ● │ GP17   │
   │ ●  GP15 │16 ●                            25 ● │ GP16   │
   │ ●  GP16 │17 ●                            24 ● │ GP15   │
   │ ●  GP17 │18 ●                            23 ● │ GP14   │
   │ ●  GP18 │19 ●                            22 ● │ GP13   │
   │ ●  GP19 │20 ●                            21 ● │ GP12   │
   └─────────┴────────────────────────────────────────┘
              │                            │
              │        [Bottom Side]       │
              └────────────────────────────┘

   ╔═══════════════════════════════════════════════════╗
   ║            ╔═══════════════════╗                   ║
   ║            ║   WiFi/BT Chip    ║ ← Antena onboard ║
   ║            ╚═══════════════════╝                   ║
   ╚═══════════════════════════════════════════════════╝

   ════════════════════════════════════════════════════
                    Pin Labels (Top View)
   ════════════════════════════════════════════════════

   LEFT SIDE                          RIGHT SIDE
   ─────────                          ──────────
   Pin  1: GP0  (UART0 TX)           Pin 40: VSYS (Voltage System)
   Pin  2: GP1  (UART0 RX)           Pin 39: VSYS
   Pin  3: GP2  (UART0 CTS)          Pin 38: 3V3_EN (3.3V Enable)
   Pin  4: GP3  (UART0 RTS)          Pin 37: 3V3 (3.3V Output)
   Pin  5: GP4  (I2C0 SDA)           Pin 36: ADC_VREF (ADC Reference)
   Pin  6: GP5  (I2C0 SCL)           Pin 35: GND
   Pin  7: GP6                       Pin 34: GP28 (ADC2) ◄── TOUCH 3
   Pin  8: GP7                       Pin 33: GP27 (ADC1) ◄── TOUCH 2
   Pin  9: GP8                       Pin 32: GP26 (ADC0) ◄── TOUCH 1
   Pin 10: GP9                       Pin 31: GP22
   Pin 11: GP10                      Pin 30: GP21
   Pin 12: GP11                      Pin 29: GP20
   Pin 13: GP12                      Pin 28: GP19
   Pin 14: GP13                      Pin 27: GP18
   Pin 15: GP14                      Pin 26: GP17
   Pin 16: GP15                      Pin 25: GP16
   Pin 17: GP16                      Pin 24: GP15
   Pin 18: GP17                      Pin 23: GP14
   Pin 19: GP18                      Pin 22: GP13
   Pin 20: GP19                      Pin 21: GP12

   ════════════════════════════════════════════════════
                    Especificaciones
   ════════════════════════════════════════════════════

   • MCU: RP2350 (Dual Core ARM Cortex-M33)
   • Flash: 4MB
   • RAM: 520KB SRAM
   • USB: USB-C 2.0
   • WiFi: 802.11n 2.4GHz
   • Bluetooth: Bluetooth 5.2
   • ADC: 4 canales (GPIO 26-29)
   • Dimensiones: 51mm × 21mm
   • Alimentación: 1.8V - 5.5V (VSYS)
   • Consumo: ~50mA típico (WiFi: ~120mA)
```

---

## Comparación de Placas

```
┌─────────────────┬─────────────────────┬─────────────────────┐
│ Característica  │ Pico 1             │ Pico 2W             │
├─────────────────┼─────────────────────┼─────────────────────┤
│ MCU             │ RP2040             │ RP2350              │
│ Cores           │ 2× Cortex-M0+      │ 2× Cortex-M33      │
│ Frecuencia      │ 133 MHz            │ 150 MHz             │
│ Flash           │ 2MB                │ 4MB                 │
│ RAM             │ 264KB              │ 520KB               │
│ USB             │ Micro-B            │ USB-C               │
│ WiFi            │ ✗                  │ ✓ 802.11n           │
│ Bluetooth       │ ✗                  │ ✓ 5.2               │
│ ADC             │ 4 canales          │ 4 canales           │
│ GPIO            │ 26                 │ 26                  │
│ Precio          │ ~$4 USD            │ ~$7 USD             │
└─────────────────┴─────────────────────┴─────────────────────┘
```

**Nota:** Para este proyecto MIDI Touch Pads, **ambas placas funcionan igual**. La diferencia principal es que el Pico 2W tiene WiFi/Bluetooth y USB-C.

---

## Conexiones

### Touch Pad 1 → GPIO 26 (Nota C4)
```
Lámina cobre ────┬──── GPIO 26 (Pin 32)
                  │
                1MΩ
                  │
                3.3V (Pin 37)
```

### Touch Pad 2 → GPIO 27 (Nota D4)
```
Lámina cobre ────┬──── GPIO 27 (Pin 33)
                  │
                1MΩ
                  │
                3.3V (Pin 37)
```

### Touch Pad 3 → GPIO 28 (Nota E4)
```
Lámina cobre ────┬──── GPIO 28 (Pin 34)
                  │
                1MΩ
                  │
                3.3V (Pin 37)
```

---

## Esquema General (Ambas Placas)

```
    ┌─────────────────────────────────────────────────────┐
    │                    PICO (1 o 2W)                    │
    │                                                     │
    │    ┌───────────────────────────────────────────┐    │
    │    │              USB (Micro-B o USB-C)         │    │
    │    └───────────────────────────────────────────┘    │
    │                                                     │
    │    Pin 32: GP26 (ADC0) ──── 1MΩ ──── 3.3V         │
    │                                   │                 │
    │                                   └──── Lámina 1   │
    │                                                     │
    │    Pin 33: GP27 (ADC1) ──── 1MΩ ──── 3.3V         │
    │                                   │                 │
    │                                   └──── Lámina 2   │
    │                                                     │
    │    Pin 34: GP28 (ADC2) ──── 1MΩ ──── 3.3V         │
    │                                   │                 │
    │                                   └──── Lámina 3   │
    │                                                     │
    └─────────────────────────────────────────────────────┘
                          │
                          │ USB
                          ▼
                      PC / DAW
```

---

## Construcción de los Touch Pads

### Opción 1: Lámina de cobre (recomendado)
1. Cortar 3 láminas de cobre de 5x5 cm
2. Limpiar con lija fino
3. Soldar cable jumper a cada lámina
4. Pegar a superficie no conductiva (cartón, madera)

### Opción 2: Paper clip / alambre
1. Formar espiral con alambre de cobre
2. Conectar cable jumper al centro
3. Montar sobre base aislante

### Opción 3: Foil tape (cinta adhesiva de aluminio)
1. Pegar cinta adhesiva de aluminio en superficie
2. Conectar cable con pinza o soldadura

---

## Cómo Funciona la Detección de Toque

```
    SIN TOQUE                          CON TOQUE
    ─────────                          ─────────

    3.3V ──── 1MΩ ──── GP26           3.3V ──── 1MΩ ──── GP26
                    │                                   │
                    │                                   │
              Capacitancia                        Capacitancia
              del pad (≈0)                         + Tu cuerpo
                    │                                   │
                    │                                   │
                   GND                                 GND

    ADC lee: ~4095                     ADC lee: ~500-1500
    (voltaje alto)                     (voltaje bajo)
```

1. **Sin toque:** El resistor de 1MΩ mantiene el pin en 3.3V → ADC lee ~4095
2. **Al tocar:** Tu cuerpo actúa como capacitancia al GND → voltaje baja → ADC lee < 1500
3. **El firmware detecta** el cambio y envía MIDI

---

## Notas Importantes

- **GPIO 26, 27, 28** son los ÚNICOS pines con ADC en ambas placas
- El **GPIO 29** también es ADC pero mide VSYS/3 (voltaje de batería)
- No usar pins USB (GP0, GP1) para touch pads
- Los resistores de 1MΩ son importantes: valores menores causan más consumo
- Mantener las láminas alejadas del cable USB para evitar interferencia
- **Pico 2W:** La antena WiFi está cerca de los pins 37-40, mantener touch pads lejos
