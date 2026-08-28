# 📚 Documentación - MIDI Touch Pads

Índice completo de documentación para el proyecto MIDI Touch Pads.

## 📖 Guías Principales

| Archivo | Descripción | Para quién |
|---------|-------------|------------|
| [MANUAL_USO.md](MANUAL_USO.md) | Manual completo de usuario | Todos |
| [HARDWARE.md](HARDWARE.md) | Diagramas y conexiones | Electrónica |
| [BUILD.md](BUILD.md) | Compilación y flasheo | Desarrolladores |
| [MIDI.md](MIDI.md) | Referencia MIDI | Desarrolladores |
| [SCALES.md](SCALES.md) | Escalas musicales | Músicos |
| [REMOTE_FLASH.md](REMOTE_FLASH.md) | Flasheo remoto | SysAdmin |
| [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | Solución de problemas | Todos |

## 🎯 Por Nivel de Experiencia

### Principiante
1. [MANUAL_USO.md](MANUAL_USO.md) - Empezar aquí
2. [HARDWARE.md](HARDWARE.md) - Armar el circuito
3. [BUILD.md](BUILD.md) - Primera compilación

### Intermedio
1. [MIDI.md](MIDI.md) - Entender MIDI
2. [SCALES.md](SCALES.md) - Escalas musicales
3. [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Solucionar problemas

### Avanzado
1. [REMOTE_FLASH.md](REMOTE_FLASH.md) - Flasheo remoto
2. Código fuente en `src/`
3. Contribuir al proyecto

## 🖼️ Recursos Visuales

| Archivo | Contenido |
|---------|-----------|
| [pico_1.jpeg](pico_1.jpeg) | Raspberry Pi Pico 1 |
| [pico2w-pinout.svg](pico2w-pinout.svg) | Pinout completo Pico 2W |
| [pico.jpeg](pico.jpeg) | Foto del Pico |
| [raspbery_pico.png](raspbery_pico.png) | Diagrama de conexión |
| [raspbery_pico_1.png](raspbery_pico_1.png) | Diagrama Pico 1 |

## 📋 Resumen Rápido

### ¿Qué necesito?
- Raspberry Pi Pico (1 o 2W)
- 3 láminas de cobre/aluminio
- 3 resistencias de 1MΩ
- Cable USB
- Raspberry Pi o PC para compilar

### ¿Cómo empiezo?
```bash
# 1. Clonar
git clone https://github.com/siliconvalleyar-oss/pico_src.git

# 2. Instalar dependencias
sudo bash script_tools/install_deps.sh

# 3. Compilar y flashear
bash script_tools/flash_full.sh
```

### ¿Qué hago si no funciona?
Ver [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
