# BUILD - Compilación y Flasheo

## Requisitos

- **CMake** 3.13 o superior
- **GCC ARM** (`arm-none-eabi-gcc`)
- **Python 3** (para herramientas del SDK)
- **Pico SDK** en `../../pico-sdk`

## Instalar Herramientas (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

O usar el script:
```bash
sudo bash install_deps.sh
```

## Estructura del Proyecto

```
pico_src/pendrive/
├── CMakeLists.txt              # CMake principal
├── pico_sdk_import.cmake       # Importador del SDK
├── .gitignore                  # Ignorar archivos generados
├── install_deps.sh             # Instala dependencias
├── flash_pendrive.sh           # Compila + flashea
├── src/
│   ├── CMakeLists.txt          # Config del proyecto
│   ├── main.c                  # Bucle principal y USB
│   ├── msc_disk.c              # Disco RAM + callbacks MSC
│   ├── msc_disk.h              # Configuración del disco
│   ├── usb_descriptors.c       # Descriptores USB (MSC + CDC)
│   └── tusb_config.h           # Config de TinyUSB
├── docs/                       # Documentación
│   ├── README.md
│   ├── HARDWARE.md
│   ├── BUILD.md
│   └── MANUAL_USO.md
└── build/                      # Directorio de build (generado)
```

## Compilación

### Primera vez
```bash
cd pico_src/pendrive
mkdir -p build
cd build
cmake ..
```

### Compilar
```bash
cd pico_src/pendrive/build
make -j$(nproc)
```

### Archivos generados
```
build/src/
├── pendrive.uf2      # Para flashear
├── pendrive.bin      # Binario raw
├── pendrive.elf      # ELF con debug
└── pendrive.elf.map  # Mapa de símbolos
```

## Flasheo al Pico

### Método 1: BOOTSEL + USB (recomendado)

1. **Desconectar** el Pico del USB
2. **Mantener presionado** el botón BOOTSEL
3. **Conectar** el Pico al USB mientras mantienes BOOTSEL
4. **Soltar** BOOTSEL cuando aparezca la unidad
5. Aparece como unidad `RPI-RP2` o `RP20xx`
6. **Copiar** el archivo `.uf2`:
   ```bash
   cp build/src/pendrive.uf2 /media/$USER/RPI-RP2/
   ```
7. El Pico se **reinicia automáticamente**
8. Conéctalo por USB y aparecerá tu pendrive

### Método 2: script
```bash
bash flash_pendrive.sh            # compila + flashea
bash flash_pendrive.sh --compile-only
bash flash_pendrive.sh --flash-only
```

### Método 3: picotool
```bash
picotool load build/src/pendrive.uf2
picotool reboot
```

## Verificar que Funciona

1. Conecta el Pico al USB de la computadora
2. Debería aparecer un **pendrive** con ~128 KB de espacio y un archivo `README.TXT`
3. Crea, copia, modifica y borra archivos normalmente

## Configurar la Capacidad

Edita `src/msc_disk.h`:

```c
#define PENDISK_BLOCK_COUNT     256u   // <- bloques de 512 bytes
```

| Bloques | Capacidad | Nota |
|---------|-----------|------|
| 128     | 64 KB     | Seguro |
| 256     | 128 KB    | **Por defecto** |
| 384     | 192 KB    | Usa casi toda la RAM |
| 512     | 256 KB    | Máximo aproximado |

> ⚠️ La RP2040 tiene **264 KB de SRAM**. No subas los bloques en exceso o el firmware no enlazará (linker out of memory).

## Rebuild Limpio

```bash
cd pico_src/pendrive/build
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

### El pendrive no aparece montado
- Revisa `dmesg | tail` y `lsblk`
- Asegúrate de flashear el firmware correcto
- El Pico debe estar alimentado por USB

### La capacidad que configuré no aparece
- El disco se formatea en `pendisk_format()` al encender
- Si cambias `PENDISK_BLOCK_COUNT`, recompila y vuelve a flashear
