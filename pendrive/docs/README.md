# 📚 Documentación - Pico Pendrive

Índice completo de documentación para el proyecto que convierte una Raspberry Pi Pico (RP2040) en un **pendrive USB** con capacidad configurable y lectura/escritura completa.

## 📖 Guías Principales

| Archivo | Descripción | Para quién |
|---------|-------------|------------|
| [MANUAL_USO.md](MANUAL_USO.md) | Manual completo de usuario | Todos |
| [HARDWARE.md](HARDWARE.md) | Requisitos de hardware y conexión | Electrónica |
| [BUILD.md](BUILD.md) | Compilación y flasheo | Desarrolladores |

## 🎯 Por Nivel de Experiencia

### Principiante
1. [MANUAL_USO.md](MANUAL_USO.md) - Empezar aquí
2. [HARDWARE.md](HARDWARE.md) - Qué necesitas
3. [BUILD.md](BUILD.md) - Primera compilación

### Intermedio
1. Código fuente en `src/`
2. Cambiar la capacidad en `src/msc_disk.h`

### Avanzado
1. Extender el formateador FAT en `src/msc_disk.c`
2. Persistir el contenido en flash QSPI
3. Añadir más interfaces USB

## 📋 Resumen Rápido

### ¿Qué necesito?
- Raspberry Pi Pico (RP2040)
- Cable USB (Micro-B)

Nada más: el pendrive es 100% software dentro del Pico.

### ¿Cómo empiezo?
```bash
# 1. Navegar al proyecto
cd pico_src/pendrive

# 2. Compilar
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 3. Flashear
cp build/src/pendrive.uf2 /media/$USER/RPI-RP2/
```

### ¿Qué hace?
Al conectarlo por USB, el Pico aparece como un **pendrive** (disco USB Mass Storage) con formato FAT, listo para copiar archivos dentro y fuera, como un pendrive real.

## ⚠️ Importante

- El medio es **RAM configurable** (por defecto **224 KB**). El espacio se configura en `src/msc_disk.h`.
- Al ser RAM **volátil**, el contenido **se pierde al apagar** el Pico. Es un *ramdisk*.
- Full **lectura/escritura**: puedes crear, modificar y borrar archivos mientras está conectado.

## ⚠️ Lectura/escritura en Linux

El firmware del Pico escribe correctamente (prueba: al cambiar el **nombre del volumen**, el label persiste al reconectar). El dispositivo es un ramdisk de lectura/escritura real.

Sin embargo, el **automontador de Linux** puede montar el volumen `vfat` en modo **solo lectura** (`ro`) porque no confía en un filesystem pre-formateado muy pequeño. En ese caso, al hacer `touch` o editar un archivo verás:

```
touch: no se puede efectuar `touch' sobre 'archivo.txt': Sistema de archivos de solo lectura
```

Para habilitar la escritura, **remonta el volumen en modo lectura-escritura**:

```bash
# 1. Identifica el pendrive del Pico (el de ~224 KB, FAT12 "PICO PENDV")
lsblk -f
df -h | grep sd

# 2. Remóntalo como lectura-escritura (ajusta el mount point del paso 1)
sudo mount -o remount,rw /media/$USER/<volumen>
# o, según cómo esté montado:
sudo mount -o remount,rw /dev/sdX1 /media/$USER/<volumen>

# 3. Ya puedes crear/editar archivos
touch /media/$USER/<volumen>/hola.txt
```

> 💡 Recordá: por ser un **ramdisk**, los archivos se pierden al apagar/desconectar el Pico. La persistencia real requiere usar la flash QSPI o una SD.
