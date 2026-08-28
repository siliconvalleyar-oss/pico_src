# BUILD - Compilación y Flasheo

## Requisitos

- **CMake** 3.13 o superior
- **GCC ARM** (`arm-none-eabi-gcc`)
- **Python 3** con `elf2uf2` del SDK
- **Pico SDK** en `../pico-sdk`

## Instalar Herramientas (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

## Estructura del Proyecto

```
pico_projects/
├── CMakeLists.txt              # CMake principal
├── pico_sdk_import.cmake       # Importador del SDK
├── midi/
│   ├── CMakeLists.txt          # Config del proyecto midi
│   ├── main.c                  # Código principal
│   ├── tusb_config.h           # Config TinyUSB (MIDI)
│   ├── usb_descriptors.c       # Descriptores USB MIDI
│   └── usb_descriptors.h       # Header
└── build/                      # Directorio de build
```

## Compilación

### Primera vez
```bash
cd pico_projects
mkdir -p build
cd build
cmake ..
```

### Compilar
```bash
cd pico_projects/build
make -j$(nproc)
```

### Output esperado
```
[100%] Built target midi
```

### Archivos generados
```
build/midi/
├── midi.uf2      # Para flashear (45 KB)
├── midi.bin      # Binario raw (23 KB)
├── midi.elf      # ELF con debug
└── midi.elf.map  # Mapa de símbolos
```

## Flasheo al Pico

### Método 1: BOOTSEL + USB (recomendado)

1. **Desconectar** el Pico del USB
2. **Mantener presionado** el botón BOOTSEL
3. **Conectar** el Pico al USB mientras mantienes BOOTSEL
4. **Soltar** BOOTSEL cuando aparezca la unidad
5. Aparece como unidad `RPI-RP2` o `RP2040`
6. **Copiar** el archivo `.uf2`:
   ```bash
   # Linux
   cp build/midi/midi.uf2 /media/$USER/RPI-RP2/
   
   # macOS
   cp build/midi/midi.uf2 /Volumes/RPI-RP2/
   
   # Windows (PowerShell)
   Copy-Item build\midi\midi.uf2 E:\
   ```
7. El Pico se **reinicia automáticamente**
8. ¡Listo! Aparece como dispositivo MIDI USB

### Método 2: picotool
```bash
# Si tienes picotool instalado
picotool load build/midi/midi.uf2
picotool reboot
```

## Verificar que Funciona

### Linux
```bash
# Verificar que el dispositivo MIDI aparece
arecord -l | grep -i midi
# o
cat /proc/asound/cards
```

### macOS
```bash
# Verificar dispositivo MIDI
system_profiler SPUSBDataType | grep -A5 -i midi
```

### Windows
- Abrir **Administrador de dispositivos**
- Buscar en "Dispositivos de sonido" → "Pico Touch MIDI"

## Rebuild Limpio

```bash
cd pico_projects/build
rm -rf *
cmake ..
make -j$(nproc)
```

## Troubleshooting de Build

### Error: "PICO_SDK_PATH not set"
```bash
export PICO_SDK_PATH=/ruta/a/pico-sdk
```

### Error: "arm-none-eabi-gcc not found"
```bash
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi
```

### Error: "TinyUSB not found"
Asegúrate que el SDK tiene TinyUSB:
```bash
ls pico-sdk/lib/tinyusb/
```
Si no existe:
```bash
cd pico-sdk
git submodule update --init lib/tinyusb
```
