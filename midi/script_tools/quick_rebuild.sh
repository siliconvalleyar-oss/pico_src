#!/bin/bash
#==============================================================================
# quick_rebuild.sh - Reconstruir rápido después de cambiar código
#==============================================================================
# Uso: bash quick_rebuild.sh [--clean]
#==============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_DIR="/home/joy/src/pico_src/midi"

print_info() {
    echo -e "${BLUE}[i]${NC} $1"
}

print_step() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Verificar si se quiere limpiar
CLEAN=false
if [[ "$1" == "--clean" ]]; then
    CLEAN=true
fi

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}              REBUILD RÁPIDO - MIDI TOUCH PADS           ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

cd "$PROJECT_DIR"

# Limpiar si se pidió
if [[ "$CLEAN" == true ]]; then
    print_info "Limpiando build anterior..."
    rm -rf build
fi

# Crear directorio build si no existe
if [[ ! -d "build" ]]; then
    mkdir -p build
fi

cd build

# Verificar si CMake ya se ejecutó
if [[ ! -f "Makefile" ]]; then
    print_info "Primera compilación, ejecutando CMake..."
    cmake ..
fi

# Compilar
print_info "Compilando..."
make -j$(nproc) 2>&1 | tail -5

# Verificar resultado
if [[ -f "src/midi.uf2" ]]; then
    SIZE=$(ls -lh src/midi.uf2 | awk '{print $5}')
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║              ¡COMPILACIÓN EXITOSA!                     ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "  Firmware: ${YELLOW}src/midi.uf2 ($SIZE)${NC}"
    echo ""
    echo -e "  Para flashear:"
    echo -e "    ${YELLOW}sudo cp src/midi.uf2 /media/joy/RPI-RP2/${NC}"
    echo ""
else
    print_error "Error: archivo .uf2 no generado"
    exit 1
fi
