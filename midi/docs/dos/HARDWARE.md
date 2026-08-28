# HARDWARE - Conexiones y Componentes

## Componentes Necesarios

| Componente | Cantidad | Descripción |
|------------|----------|-------------|
| Raspberry Pi Pico | 1 | RP2040 con USB |
| Lámina de cobre/aluminio | 3 | Touch pads (5x5 cm mínimo) |
| Resistencia 1MΩ | 3 | Pull-up para cada pad |
| Cable jumper M-M | 6 | Conexiones |
| USB cable micro-B | 1 | Programación y MIDI |

## Pines del Pico

```
         ┌─────────────────────┐
    VB   │ 1  ○            40 ○│ VSYS
   GP28  │ 2  ○            39 ○│ VSYS
   GP27  │ 3  ○            38 ○│ 3V3_EN
   GP26  │ 4  ○            37 ○│ 3V3
   ADC   │ 5  ○            36 ○│ ADC_VREF
    GND  │ 6  ○            35 ○│ GND
    GP22 │ 7  ○            34 ○│ GP28 (ADC2)
    GP21 │ 8  ○            33 ○│ GP27 (ADC1)
    GP20 │ 9  ○            32 ○│ GP26 (ADC0) ◄── TOUCH 1
    GP19 │ 10 ○            31 ○│ GP22
    GP18 │ 11 ○            30 ○│ GP21
    GP17 │ 12 ○            29 ○│ GP20
    GP16 │ 13 ○            28 ○│ GP19
    GP15 │ 14 ○            27 ○│ GP18
    GP14 │ 15 ○            26 ○│ GP17
    GP13 │ 16 ○            25 ○│ GP16
    GP12 │ 17 ○            24 ○│ GP15
    GP11 │ 18 ○            23 ○│ GP14
    GP10 │ 19 ○            22 ○│ GP13
     GP9 │ 20 ○            21 ○│ GP12
         └─────────────────────┘
              USB connector
```

## Conexiones

### Touch Pad 1 → GPIO 26 (Nota C4)
```
Lámina cobre ────┬──── GPIO 26 (Pin 4)
                  │
                1MΩ
                  │
                3.3V (Pin 36)
```

### Touch Pad 2 → GPIO 27 (Nota D4)
```
Lámina cobre ────┬──── GPIO 27 (Pin 3)
                  │
                1MΩ
                  │
                3.3V (Pin 36)
```

### Touch Pad 3 → GPIO 28 (Nota E4)
```
Lámina cobre ────┬──── GPIO 28 (Pin 2)
                  │
                1MΩ
                  │
                3.3V (Pin 36)
```

## Esquema General

```
    Pico                          Touch Pads
    ┌─────┐
    │     │ GPIO 26 ──── 1MΩ ──── 3.3V
    │     │                     │
    │     │                     └──── Lámina 1 (C4)
    │     │
    │     │ GPIO 27 ──── 1MΩ ──── 3.3V
    │     │                     │
    │     │                     └──── Lámina 2 (D4)
    │     │
    │     │ GPIO 28 ──── 1MΩ ──── 3.3V
    │     │                     │
    │     │                     └──── Lámina 3 (E4)
    │     │
    │ USB │─────────────────── PC
    └─────┘
```

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

## Cómo Funciona la Detección de Toque

1. Sin toque: El resistor de 1MΩ mantiene el pin en 3.3V → ADC lee ~4095
2. Al tocar: Tu cuerpo actúa como capacitancia al GND → voltaje baja → ADC lee < 1500
3. El firmware detecta el cambio y envía MIDI

## Notas Importantes

- **GPIO 26, 27, 28** son los ÚNICOS pines con ADC en el Pico
- El **GPIO 29** también es ADC pero mide VSYS/3 (voltaje de batería)
- No usar pins USB (GP0, GP1) para touch pads
- Los resistores de 1MΩ son importantes: valores menores causan más consumo
- Mantener las láminas alejadas del cable USB para evitar interferencia
