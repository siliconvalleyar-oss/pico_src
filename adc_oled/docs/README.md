# 📚 Documentación - ADC + OLED + KY-037

Índice completo de documentación para el proyecto de detección de sonido con KY-037 y display OLED SSD1306.

## 📖 Guías Principales

| Archivo | Descripción | Para quién |
|---------|-------------|------------|
| [MANUAL_USO.md](MANUAL_USO.md) | Manual completo de usuario | Todos |
| [HARDWARE.md](HARDWARE.md) | Diagramas y conexiones | Electrónica |
| [BUILD.md](BUILD.md) | Compilación y flasheo | Desarrolladores |

## 🎯 Por Nivel de Experiencia

### Principiante
1. [MANUAL_USO.md](MANUAL_USO.md) - Empezar aquí
2. [HARDWARE.md](HARDWARE.md) - Armar el circuito
3. [BUILD.md](BUILD.md) - Primera compilación

### Intermedio
1. Código fuente en `src/`
2. Modificar parámetros de display y ADC

### Avanzado
1. Extender con más canales ADC (GPIO 27, 28)
2. Agregar botones para cambiar modo de visualización

## 📋 Resumen Rápido

### ¿Qué necesito?
- Raspberry Pi Pico (RP2040)
- Módulo KY-037 (sensor de sonido)
- Display OLED SSD1306 128x64 (I2C)
- Cable USB
- Protoboard y cables jumper

### ¿Cómo empiezo?
```bash
# 1. Navegar al proyecto
cd pico_src/adc_oled

# 2. Compilar
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 3. Flashear
cp build/adc_oled.uf2 /media/$USER/RPI-RP2/
```

### ¿Qué hace?
Lee el nivel de sonido desde el módulo KY-037 y lo muestra en el OLED en tiempo real.
