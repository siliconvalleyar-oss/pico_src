# Manual de Uso - Pico Pendrive

Convierte tu Raspberry Pi Pico (RP2040) en un **pendrive USB** de capacidad configurable, con lectura y escritura completas.

---

## 1. ¿Qué es esto?

Un "pendrive" construido íntegramente con una Raspberry Pi Pico. Al conectarla por USB aparece en la computadora como un disco normal (formato FAT) donde puedes copiar, modificar y borrar archivos, igual que con un pendrive de verdad.

La diferencia es que el medio de almacenamiento es la **RAM configurable** del chip:

- **Espacio configurable**: eliges los bloques en `src/msc_disk.h`.
- **Lectura/escritura completa**: es un disco de verdad mientras está encendido.
- **Contenido volátil**: al apagar el Pico, todo se borra (es un ramdisk).

---

## 2. Componentes Necesarios

| Componente | Cantidad | Dónde conseguirlo |
|------------|----------|-------------------|
| Raspberry Pi Pico | 1 | Tiendas de electrónica (~$4 USD) |
| Cable USB Micro-B | 1 | El que trae el Pico |

**Total aproximado:** ~$4 USD

---

## 3. Instalar el Firmware

### Requisitos previos

- Computadora con Windows, macOS o Linux
- Cable USB
- Pico SDK instalado

### Paso 1: Compilar

```bash
cd pico_src/pendrive
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### Paso 2: Poner el Pico en modo programación

1. **Desconectar** el Pico del USB
2. **Mantener presionado** el botón BOOTSEL
3. **Conectar** el Pico al USB mientras mantienes el botón
4. **Soltar** el botón cuando la luz LED se encienda

Aparece una unidad USB llamada `RPI-RP2` o `RP20xx`.

### Paso 3: Copiar el firmware

**Windows:**
```
Copia el archivo build/src/pendrive.uf2 a la unidad RPI-RP2
```

**macOS:**
```bash
cp pico_src/pendrive/build/src/pendrive.uf2 /Volumes/RPI-RP2/
```

**Linux:**
```bash
cp pico_src/pendrive/build/src/pendrive.uf2 /media/$USER/RPI-RP2/
```

### Paso 4: Reiniciar

El Pico se reinicia automáticamente. Cuando lo conectes por USB, aparecerá tu pendrive.

---

## 4. Cómo Usar

### Montar el pendrive

1. Conecta el Pico a la computadora por USB
2. Espera unos segundos
3. Busca el disco en el sistema de archivos (se llamará **"PICO PENDV"**)

**Linux:**
```bash
# Ver el disco
lsblk
# Montar (si no se auto-monta)
sudo mount /dev/sdX1 /mnt/pendrive
# Copiar algo dentro
cp datos.txt /mnt/pendrive/
```

**Windows:**
- Abre el Explorador de archivos
- El disco aparece como una unidad de almacenamiento
- Copia y pega archivos normalmente

**macOS:**
- Aparece en el escritorio / Finder
- Arrastra archivos dentro y fuera

### Qué contiene al inicio

El pendrive se formatea automáticamente al encender y trae un archivo de bienvenida:
```
README.TXT   -> información del dispositivo
```

### Escribir y borrar

- **Copiar**: arrastra archivos a la unidad (vive en RAM)
- **Borrar**: elimínalos normalmente
- **Formatear**: el SO puede formatearlo (volverá a formatearse al reiniciar)

---

## 5. Configurar la Capacidad

Edita `src/msc_disk.h`:

```c
#define PENDISK_BLOCK_COUNT     256u   // <- bloques de 512 bytes
```

Ejemplos:

| Valores | Capacidad | Uso típico |
|---------|-----------|------------|
| `128u`  | 64 KB     | Notas, config |
| `256u`  | 128 KB    | **Por defecto** |
| `384u`  | 192 KB    | Más espacio |
| `512u`  | 256 KB    | Máximo en RAM |

Luego recompila y vuelve a flashear.

> ⚠️ La RP2040 solo tiene 264 KB de SRAM; no excedas los ~480 bloques.

---

## 6. Monitor Serial USB (CDC)

El Pico también expone un puerto serial virtual para diagnóstico:

| Sistema | Puerto |
|---------|--------|
| Linux | `/dev/ttyACM0` |
| Windows | `COMx` |
| macOS | `/dev/cu.usbmodem...` |

```
$ minicom -D /dev/ttyACM0 -b 115200
========================================
  Pico Pendrive
  Raspberry Pi Pico RP2040
========================================
[SYS] RAM disk formatted
[SYS] Capacity: 128 KB (256 x 512B blocks)
[SYS] Ready. Connect USB to mount the pendrive.
```

Comandos disponibles:

| Comando | Resultado |
|---------|-----------|
| `INFO` | Muestra estado del disco (bloques, tamaño, mounted/ejected) |

---

## 7. Solución de Problemas

### El pendrive no aparece

1. **Verificar firmware:** que `pendrive.uf2` esté flasheado
2. **Reconectar:** desenchufa y vuelve a conectar el Pico
3. **Revisar dmesg:** `dmesg | tail` (Linux)
4. **Otro cable:** algunos cables USB solo cargan (sin datos)

### El disco muestra menos espacio del esperado

- El sistema operativo descuenta el espacio de FAT y directorio raíz
- Revisa `PENDISK_BLOCK_COUNT` y recompila si lo cambiaste

### Al reiniciar perdí los archivos

- Es **normal**: el medio es RAM volátil (un ramdisk)
- Para persistir entre reinicios se necesitaría flash QSPI (limitado en cantidad de escrituras)

### El LED parpadea de forma distinta

- **250 ms**: no está montado (sin USB)
- **1000 ms**: montado y funcionando
- **2500 ms**: USB en suspensión

---

## 8. Especificaciones Técnicas

| Parámetro | Valor |
|-----------|-------|
| MCU | RP2040 (dual core ARM Cortex-M0+) |
| Frecuencia | 125 MHz |
| SRAM | 264 KB (disco usa parte de ella) |
| Interfaz USB | USB 2.0 Full Speed |
| Clase USB | Mass Storage (MSC) + CDC |
| Formato | FAT12 / FAT16 |
| Capacidad por defecto | 128 KB |
| Lectura/Escritura | Sí |
| Persistencia | No (RAM volátil) |
| Consumo típico | ~20-40 mA |

---

## 9. Limitaciones

- **RAM volátil**: los archivos se pierden al apagar
- **Capacidad limitada** por los 264 KB de SRAM del RP2040
- **USB Full Speed** (no High Speed), lecturas sobre ~1 MB/s
- Sin protección de energía / sobretensión de un pendrive físico

---

## 10. Próximas Mejoras Posibles

- [ ] Persistir archivos en la flash QSPI del Pico
- [ ] Interfaz web/MIDI adicional
- [ ] Soportar nombres largos (VFAT)
- [ ] Agregar doble LUN (dos discos)
- [ ] Botón para "expulsar" de forma segura

---

## 11. Licencia

MIT License - Libre para uso personal y comercial.
