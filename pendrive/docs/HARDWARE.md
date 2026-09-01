# HARDWARE - Requisitos y Conexión

## Componentes Necesarios

| Componente | Cantidad | Descripción |
|------------|----------|-------------|
| Raspberry Pi Pico (RP2040) | 1 | La placa que actúa de pendrive |
| Cable USB Micro-B | 1 | Conexión de datos y alimentación |

No se necesita ningún otro componente: el "disco" es 100% software dentro del Pico.

---

## Raspberry Pi Pico (RP2040)

### Por qué funciona

Toda la lógica del pendrive corre dentro del chip:
- **USB** nativo del RP2040 (con TinyUSB) emula una unidad de Mass Storage.
- **SRAM** (264 KB) actúa como el medio de almacenamiento configurable.
- La CPU formatea una partición FAT dentro de la RAM al encender.

```
              ┌─────────────────────────────┐
              │        RASPBERRY PI PICO     │
              │                             │
              │  ┌───────────────────────┐  │
              │  │   RP2040 (SRAM 264KB) │  │
              │  │   - Disco RAM FAT     │  │
              │  │   - TinyUSB MSC       │  │
              │  └──────────┬────────────┘  │
              │             │  USB          │
              └─────────────┼───────────────┘
                            │
                       Cable Micro-USB
                            │
                         PC / Host
                      (aparece un disco)
```

### Diagrama de Pines

No se usan pines GPIO para este proyecto. Solo se utiliza el puerto **USB Micro-B** (datos + alimentación).

```
                     RASPBERRY PI PICO
                          ┌───────┐
                          │ USB   │◄── Cable al PC (datos + 5V)
                          │ Micro │
                          └───────┘
```

---

## Conexión USB

| Función | Puerto USB | Uso |
|---------|------------|-----|
| Alimentación | USB Micro-B | 5V desde PC |
| Datos MSC | USB Micro-B | El host ve el disco |
| Datos CDC | USB Micro-B | Puerto serial virtual (diagnóstico) |

Solamente conecta el cable Micro-B entre el Pico y la computadora.

---

## Esquema General

```
    ┌──────────────────────────────────────────┐
    │              RASPBERRY PI PICO           │
    │                                          │
    │     USB Micro-B ──────┬─────────  PC     │
    │                       │                 │
    │        (Solo USB,   comunicación        │
    │         sin GPIO)    y alimentación)    │
    └──────────────────────────────────────────┘
```

---

## Especificaciones del Disco (por defecto)

| Parámetro | Valor |
|-----------|-------|
| Capacidad | 224 KB (448 bloques × 512 B) |
| Formato | FAT12 / FAT16 (auto) |
| Lectura/Escritura | Completa |
| Persistencia | No (RAM volátil) |
| Interfaz | USB 2.0 Full Speed |

---

## Notas Importantes

- **No requieres** componentes externos, protoboard ni jumpers.
- El contenido del "pendrive" se **borra cada vez que se apaga** porque vive en RAM.
- Si usas un **Pico W / Pico 2**, el mismo firmware funciona (misma SRAM/diferente capacidad).
- Máximo teórico limitado por los 264 KB de SRAM del RP2040.
