# Flasheo Remoto de Firmware

Guía completa para compilar y cargar firmware en el Raspberry Pi Pico de forma remota via SSH.

## Requisitos

- Raspberry Pi con acceso SSH
- Pico conectado al USB de la Raspberry
- Herramientas instaladas (ver `install_deps.sh`)

## Método 1: Picotool (Recomendado)

### Instalar picotool

```bash
# En la Raspberry Pi
sudo apt install picotool
```

### Cargar firmware directamente

```bash
# Si el Pico está en modo BOOTSEL
picotool load /home/joy/src/pico_src/midi/build/src/midi.uf2 -f

# Reiniciar en modo BOOTSEL primero (si ya está corriendo)
picotool reboot -u
sleep 2
picotool load /home/joy/src/pico_src/midi/build/src/midi.uf2 -f
```

### Comandos útiles de picotool

```bash
# Ver información del Pico
picotool info

# Reiniciar el Pico
picotool reboot

# Reiniciar en modo BOOTSEL
picotool reboot -u

# Cargar firmware (forzar)
picotool load firmware.uf2 -f

# Cargar firmware (solo si cambió)
picotool load firmware.uf2
```

## Método 2: Copia Manual (Mount/Unmount)

### Paso 1: Identificar el dispositivo

```bash
lsusb | grep 2e8a
# Output: Bus 001 Device 005: ID 2e8a:0003 Raspberry Pi RP2 Boot

lsblk | grep -A2 sda
# Output: sda      8:0    1   14M  0 disk
#         └─sda1   8:1    1   14M  0 part /media/joy/RPI-RP2
```

### Paso 2: Montar y copiar

```bash
# Crear punto de montaje
sudo mkdir -p /mnt/pico

# Montar
sudo mount /dev/sda1 /mnt/pico

# Copiar firmware
sudo cp /home/joy/src/pico_src/midi/build/src/midi.uf2 /mnt/pico/

# Sincronizar y desmontar
sync
sudo umount /mnt/pico
```

### Paso 3: Reiniciar

El Pico se reinicia automáticamente después de recibir el .uf2

## Script Automatizado

### flash_pico.sh

```bash
#!/bin/bash
# flash_pico.sh - Compilar y flashear firmware al Pico

set -e

PROJECT_DIR="/home/joy/src/pico_src/midi"
FIRMWARE="$PROJECT_DIR/build/src/midi.uf2"

echo "=== Compilando proyecto ==="
cd "$PROJECT_DIR"
rm -rf build
mkdir build
cd build
cmake .. 2>&1 | tail -3
make -j$(nproc) 2>&1 | tail -3

echo ""
echo "=== Verificando firmware ==="
if [[ ! -f "$FIRMWARE" ]]; then
    echo "Error: Firmware no generado"
    exit 1
fi
ls -lh "$FIRMWARE"

echo ""
echo "=== Buscando Pico ==="

# Verificar si picotool está disponible
if command -v picotool &> /dev/null; then
    # Método picotool
    echo "Usando picotool..."
    
    # Verificar si el Pico está en BOOTSEL
    if lsusb | grep -q "2e8a:0003"; then
        picotool load "$FIRMWARE" -f
        echo "Firmware cargado con picotool"
    else
        echo "Reiniciando Pico en modo BOOTSEL..."
        picotool reboot -u
        sleep 3
        picotool load "$FIRMWARE" -f
        echo "Firmware cargado con picotool"
    fi
else
    # Método manual
    echo "Usando copia manual..."
    
    # Buscar dispositivo
    if lsusb | grep -q "2e8a:0003"; then
        sudo mkdir -p /mnt/pico
        sudo mount /dev/sda1 /mnt/pico
        sudo cp "$FIRMWARE" /mnt/pico/
        sync
        sudo umount /mnt/pico
        echo "Firmware copiado"
    else
        echo "Pico no encontrado en modo BOOTSEL"
        echo "Conecte el Pico manteniendo BOOTSEL"
        exit 1
    fi
fi

echo ""
echo "=== ¡Listo! ==="
sleep 2
lsusb | grep -i "cafe\|2e8a"
```

## Solución de Problemas

### "Permission denied" al montar

```bash
# Usar sudo
sudo mount /dev/sda1 /mnt/pico
sudo cp firmware.uf2 /mnt/pico/
sudo umount /mnt/pico
```

### "Device or resource busy"

```bash
# Forzar desmontaje
sudo umount -l /mnt/pico

# O matar procesos que usen el dispositivo
sudo fuser -k /mnt/pico
```

### Pico no aparece en BOOTSEL

1. Desconectar el Pico
2. Mantener presionado BOOTSEL
3. Conectar al USB
4. Soltar BOOTSEL después de 2 segundos

### picotool no encuentra el Pico

```bash
# Verificar que el Pico está conectado
lsusb | grep 2e8a

# Reiniciar el servicio USB
sudo systemctl restart udev
```

## Flujo Completo (Script Único)

```bash
#!/bin/bash
# todo_en_uno.sh - Compilar, flashear y verificar

set -e

echo "=========================================="
echo "  MIDI TOUCH PADS - FLASH COMPLETO"
echo "=========================================="

# 1. Compilar
echo ""
echo "[1/4] Compilando..."
cd /home/joy/src/pico_src/midi
rm -rf build && mkdir build && cd build
cmake .. 2>&1 | tail -2
make -j$(nproc) 2>&1 | tail -2

# 2. Verificar firmware
echo ""
echo "[2/4] Verificando firmware..."
FIRMWARE="src/midi.uf2"
if [[ ! -f "$FIRMWARE" ]]; then
    echo "ERROR: Firmware no generado"
    exit 1
fi
ls -lh "$FIRMWARE"

# 3. Flashear
echo ""
echo "[3/4] Flasheando al Pico..."
if command -v picotool &> /dev/null; then
    if ! lsusb | grep -q "2e8a:0003"; then
        picotool reboot -u
        sleep 3
    fi
    picotool load "$FIRMWARE" -f
else
    if ! lsusb | grep -q "2e8a:0003"; then
        echo "ERROR: Pico no está en BOOTSEL"
        echo "Reinicie manualmente con BOOTSEL presionado"
        exit 1
    fi
    sudo mkdir -p /mnt/pico
    sudo mount /dev/sda1 /mnt/pico
    sudo cp "$FIRMWARE" /mnt/pico/
    sync
    sudo umount /mnt/pico
fi

# 4. Verificar
echo ""
echo "[4/4] Verificando dispositivo..."
sleep 3
if lsusb | grep -q "cafe:4015"; then
    echo ""
    echo "=========================================="
    echo "  ¡FIRMWARE CARGADO EXITOSAMENTE!"
    echo "=========================================="
    lsusb | grep cafe
else
    echo "WARNING: Dispositivo MIDI no detectado"
    echo "Espere unos segundos y verifique con:"
    echo "  lsusb | grep cafe"
fi
```
