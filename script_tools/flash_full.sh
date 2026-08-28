#!/bin/bash
#==============================================================================
# flash_full.sh - Compilar y flashear firmware al Raspberry Pi Pico
#==============================================================================
# Uso: bash flash_full.sh [--compile-only] [--flash-only]
#==============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_DIR="/home/joy/src/pico_src/midi"
BUILD_DIR="$PROJECT_DIR/build"
FIRMWARE="$BUILD_DIR/src/midi.uf2"
PICOTOOL="/home/joy/src/pico/picotool/build/picotool"

# Parse arguments
COMPILE_ONLY=false
FLASH_ONLY=false

for arg in "$@"; do
    case $arg in
        --compile-only) COMPILE_ONLY=true ;;
        --flash-only) FLASH_ONLY=true ;;
    esac
done

print_header() {
    echo ""
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║         FLASH COMPLETO - RASPBERRY PI PICO             ║${NC}"
    echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

print_step() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[i]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Step 1: Compile
compile_project() {
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}                    PASO 1: COMPILAR                      ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    
    cd "$PROJECT_DIR"
    
    # Clean and create build directory
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    print_info "Ejecutando CMake..."
    if cmake .. 2>&1 | tail -3; then
        print_step "CMake configurado"
    else
        print_error "Error en CMake"
        exit 1
    fi
    
    print_info "Compilando..."
    if make -j$(nproc) 2>&1 | tail -5; then
        print_step "Compilación exitosa"
    else
        print_error "Error en compilación"
        exit 1
    fi
    
    # Verify firmware
    if [[ -f "$FIRMWARE" ]]; then
        SIZE=$(ls -lh "$FIRMWARE" | awk '{print $5}')
        print_step "Firmware generado: midi.uf2 ($SIZE)"
    else
        print_error "Firmware no generado"
        exit 1
    fi
}

# Step 2: Flash
flash_firmware() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}                    PASO 2: FLASHEAR                      ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    
    # Check if firmware exists
    if [[ ! -f "$FIRMWARE" ]]; then
        print_error "Firmware no encontrado: $FIRMWARE"
        print_info "Primero compile el proyecto"
        exit 1
    fi
    
    # Check for picotool
    if [[ -x "$PICOTOOL" ]]; then
        print_info "Usando picotool local..."
        
        # Check if Pico is in BOOTSEL
        if lsusb | grep -q "2e8a:0003"; then
            print_step "Pico en modo BOOTSEL detectado"
            $PICOTOOL load "$FIRMWARE" -f
            print_step "Firmware cargado con picotool"
        else
            print_info "Reiniciando Pico en modo BOOTSEL..."
            $PICOTOOL reboot -u
            sleep 3
            
            if lsusb | grep -q "2e8a:0003"; then
                $PICOTOOL load "$FIRMWARE" -f
                print_step "Firmware cargado con picotool"
            else
                print_error "No se pudo reiniciar el Pico"
                exit 1
            fi
        fi
    elif command -v picotool &> /dev/null; then
        print_info "Usando picotool del sistema..."
        
        if lsusb | grep -q "2e8a:0003"; then
            picotool load "$FIRMWARE" -f
        else
            picotool reboot -u
            sleep 3
            picotool load "$FIRMWARE" -f
        fi
    else
        print_info "Usando copia manual..."
        
        # Check if Pico is in BOOTSEL
        if ! lsusb | grep -q "2e8a:0003"; then
            print_error "Pico no está en modo BOOTSEL"
            echo ""
            echo "  Para entrar en modo BOOTSEL:"
            echo "  1. Desconectar el Pico"
            echo "  2. Mantener presionado BOOTSEL"
            echo "  3. Conectar al USB"
            echo "  4. Soltar BOOTSEL después de 2 segundos"
            echo ""
            exit 1
        fi
        
        print_step "Pico en modo BOOTSEL detectado"
        
        # Mount and copy
        sudo mkdir -p /mnt/pico
        sudo mount /dev/sda1 /mnt/pico
        sudo cp "$FIRMWARE" /mnt/pico/
        sync
        sudo umount /mnt/pico
        
        print_step "Firmware copiado"
    fi
}

# Step 3: Verify
verify_device() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}                    PASO 3: VERIFICAR                     ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    
    print_info "Esperando reinicio del Pico..."
    sleep 3
    
    if lsusb | grep -q "cafe:4015"; then
        echo ""
        echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}║           ¡FIRMWARE CARGADO EXITOSAMENTE!              ║${NC}"
        echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
        echo ""
        lsusb | grep cafe
        echo ""
    else
        print_warning "Dispositivo MIDI no detectado aún"
        print_info "Espere unos segundos y verifique con: lsusb | grep cafe"
    fi
}

#══════════════════════════════════════════════════════════════
# MAIN
#══════════════════════════════════════════════════════════════

print_header

if [[ "$FLASH_ONLY" == true ]]; then
    flash_firmware
    verify_device
elif [[ "$COMPILE_ONLY" == true ]]; then
    compile_project
else
    compile_project
    flash_firmware
    verify_device
fi

echo ""
print_step "Proceso completado"
echo ""
