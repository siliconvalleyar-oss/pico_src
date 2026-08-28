#!/bin/bash
#==============================================================================
# flash.sh - Flashear firmware al Raspberry Pi Pico
#==============================================================================
# Uso: bash flash.sh
#==============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_DIR="/home/joy/src/pico_src/midi"
FIRMWARE="$PROJECT_DIR/build/src/midi.uf2"

print_info() {
    echo -e "${BLUE}[i]${NC} $1"
}

print_step() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

echo ""
echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║           FLASHEAR FIRMWARE - RASPBERRY PI PICO        ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""

# Verificar que existe el firmware
if [[ ! -f "$FIRMWARE" ]]; then
    print_error "Firmware no encontrado: $FIRMWARE"
    echo ""
    print_info "Primero compile el proyecto:"
    echo "  cd $PROJECT_DIR"
    echo "  bash script_tools/fix_first_build.sh"
    exit 1
fi

SIZE=$(ls -lh "$FIRMWARE" | awk '{print $5}')
print_step "Firmware encontrado: midi.uf2 ($SIZE)"

# Buscar unidad RPI-RP2
print_info "Buscando unidad RPI-RP2..."

RPI_MOUNT=""
for dir in /media/joy/RPI-RP2 /media/joy/RP2040 /mnt/joy/RPI-RP2; do
    if [[ -d "$dir" ]]; then
        RPI_MOUNT="$dir"
        break
    fi
done

# También buscar por dispositivo de bloque
if [[ -z "$RPI_MOUNT" ]]; then
    for dev in /dev/sd*; do
        if lsblk -o LABEL | grep -q "RPI-RP2\|RP2040" 2>/dev/null; then
            MOUNT_POINT="/mnt/rpi_pico"
            mkdir -p "$MOUNT_POINT"
            sudo mount "$dev" "$MOUNT_POINT" 2>/dev/null && RPI_MOUNT="$MOUNT_POINT"
            break
        fi
    done
fi

if [[ -z "$RPI_MOUNT" ]]; then
    print_warning "Unidad RPI-RP2 no encontrada"
    echo ""
    echo -e "${YELLOW}Pasos para entrar en modo BOOTSEL:${NC}"
    echo "  1. DESCONECTAR el Pico del USB"
    echo "  2. Mantener presionado el botón BOOTSEL"
    echo "  3. CONECTAR el Pico al USB mientras mantienes BOOTSEL"
    echo "  4. SOLTAR el botón cuando la LED se encienda"
    echo ""
    echo -e "${YELLOW}Espere 5 segundos y vuelva a ejecutar este script...${NC}"
    echo ""
    
    # Esperar a que aparezca la unidad
    read -p "Presione Enter cuando el Pico esté en modo BOOTSEL..."
    
    for dir in /media/joy/RPI-RP2 /media/joy/RP2040 /mnt/joy/RPI-RP2; do
        if [[ -d "$dir" ]]; then
            RPI_MOUNT="$dir"
            break
        fi
    done
    
    if [[ -z "$RPI_MOUNT" ]]; then
        print_error "No se pudo encontrar la unidad RPI-RP2"
        print_info "Verifique que el Pico esté conectado y en modo BOOTSEL"
        exit 1
    fi
fi

print_step "Unidad encontrada: $RPI_MOUNT"

# Copiar firmware
print_info "Copiando firmware..."
sudo cp "$FIRMWARE" "$RPI_MOUNT/"
print_step "Firmware copiado exitosamente"

# Verificar
if [[ -f "$RPI_MOUNT/midi.uf2" ]]; then
    COPIED_SIZE=$(ls -lh "$RPI_MOUNT/midi.uf2" | awk '{print $5}')
    print_step "Verificación exitosa: midi.uf2 ($COPIED_SIZE)"
else
    print_error "Error al verificar la copia"
    exit 1
fi

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║              ¡FLASHEO COMPLETADO!                      ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "El Pico se reiniciará automáticamente."
echo -e "Verifique el dispositivo MIDI con:"
echo -e "  ${YELLOW}lsusb | grep -i cafe${NC}"
echo ""
