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

## Estructura del Proyecto

```
pico_src/adc_oled/
├── CMakeLists.txt              # CMake principal
├── pico_sdk_import.cmake       # Importador del SDK
├── .gitignore                  # Ignorar archivos generados
├── src/
│   ├── CMakeLists.txt          # Config del proyecto
│   └── main.c                  # Código principal
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
cd pico_src/adc_oled
mkdir -p build
cd build
cmake ..
```

### Compilar
```bash
cd pico_src/adc_oled/build
make -j$(nproc)
```

### Output esperado
```
[100%] Built target adc_oled
```

### Archivos generados
```
build/adc_oled/
├── adc_oled.uf2      # Para flashear
├── adc_oled.bin      # Binario raw
├── adc_oled.elf      # ELF con debug
└── adc_oled.elf.map  # Mapa de símbolos
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
   cp build/adc_oled/adc_oled.uf2 /media/$USER/RPI-RP2/

   # macOS
   cp build/adc_oled/adc_oled.uf2 /Volumes/RPI-RP2/

   # Windows (PowerShell)
   Copy-Item build\adc_oled\adc_oled.uf2 E:\
   ```
7. El Pico se **reinicia automáticamente**
8. ¡Listo! El display OLED mostrará los valores del ADC.

### Método 2: picotool
```bash
# Si tienes picotool instalado
picotool load build/adc_oled/adc_oled.uf2
picotool reboot
```

## Verificar que Funciona

1. Conectar el KY-037 y el OLED según [HARDWARE.md](HARDWARE.md)
2. Al detectar sonido, el OLED debe mostrar:
   - **ADC**: valor analógico de 0 a 4095
   - **V**: voltaje medido (0.00V a 3.30V)
   - **DO**: estado digital (HIGH/LOW)
   - **Barra**: representación gráfica del nivel de sonido

## Rebuild Limpio

```bash
cd pico_src/adc_oled/build
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

### El OLED no muestra nada
- Verificar conexiones I2C (SDA, SCL, VCC, GND)
- Verificar dirección I2C (0x3C o 0x3D)
- Usar un scanner I2C para detectar el dispositivo

### El KY-037 no detecta sonido
- Verificar conexiones del KY-037 (VCC, GND, AO, DO)
- Ajustar el potenciómetro de sensibilidad del KY-037
- Verificar que el código use el GPIO correcto (GPIO 27 para AO, GPIO 26 para DO)
