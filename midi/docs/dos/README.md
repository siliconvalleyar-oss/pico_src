# MIDI Touch Pads - Documentación

Convierte tu Raspberry Pi Pico en un controlador MIDI USB con touch pads analógicos.

## Archivos de Documentación

| Archivo | Contenido |
|---------|-----------|
| [HARDWARE.md](HARDWARE.md) | Conexiones, componentes y esquemas |
| [BUILD.md](BUILD.md) | Compilación y flasheo del firmware |
| [MIDI.md](MIDI.md) | Referencia del protocolo MIDI implementado |
| [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | Solución de problemas comunes |

## Resumen del Proyecto

- **MCU:** Raspberry Pi Pico (RP2040)
- **Entradas:** 4 touch pads en GPIO 26, 27, 28 (ADC)
- **Salida:** USB MIDI (Note On/Off, CC)
- **Notas:** C4, D4, E4, G4

## Inicio Rápido

1. Armar el circuito (ver HARDWARE.md)
2. Compilar el firmware (ver BUILD.md)
3. Flashear al Pico
4. Conectar un sintetizador MIDI
5. ¡Tocar los pads!

## Requisitos

- Raspberry Pi Pico (o Pico W)
- 3 láminas de cobre/aluminio (touch pads)
- 3 resistores de 1MΩ
- Cables jumper
- USB cable
- PC con sintetizador MIDI instalado
