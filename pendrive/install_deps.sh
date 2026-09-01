#!/bin/bash
#==============================================================================
# install_deps.sh - Instalar dependencias para el proyecto pendrive
#==============================================================================
# Uso: sudo bash install_deps.sh
#==============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo ""
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║     INSTALADOR - PICO PENDRIVE - RASPBERRY PI           ║${NC}"
    echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
    echo ""
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

print_info() {
    echo -e "${BLUE}[i]${NC} $1"
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "Este script debe ejecutarse como root"
        echo "Uso: sudo bash $0"
        exit 1
    fi
}

update_repos() {
    print_step "Actualizando repositorios..."
    apt-get update -qq
}

install_build_tools() {
    print_step "Instalando herramientas de compilación..."
    apt-get install -y \
        build-essential \
        cmake \
        git \
        python3 \
        python3-pip \
        pkg-config \
        libusb-1.0-0-dev
    print_step "Herramientas de compilación instaladas"
}

install_arm_compiler() {
    print_step "Instalando compilador ARM..."

    if command -v arm-none-eabi-gcc &> /dev/null; then
        VERSION=$(arm-none-eabi-gcc --version | head -n1)
        print_warning "Ya instalado: $VERSION"
    else
        apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi
        print_step "Compilador ARM instalado"
    fi
}

verify_picotool() {
    print_step "Verificando picotool..."

    PICOTOOL_DIR="/mnt/disk/src/rpico/picotool"
    if [[ -x "$PICOTOOL_DIR/build/picotool" ]]; then
        VERSION=$("$PICOTOOL_DIR/build/picotool" version 2>/dev/null || echo "desconocida")
        print_step "picotool encontrado: $PICOTOOL_DIR/build/picotool ($VERSION)"
    else
        print_warning "picotool no compilado en $PICOTOOL_DIR"
        print_info "Compila picotool con:"
        echo "  cd $PICOTOOL_DIR"
        echo "  mkdir -p build && cd build"
        echo "  cmake .. && make -j\$(nproc)"
    fi
}

verify_install() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    print_step "Verificando instalación..."
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"

    command -v arm-none-eabi-gcc &> /dev/null && print_step "ARM GCC: $(arm-none-eabi-gcc --version | head -n1)" || print_error "ARM GCC no encontrado"
    command -v cmake &> /dev/null && print_step "CMake: $(cmake --version | head -n1)" || print_error "CMake no encontrado"
    command -v make &> /dev/null && print_step "Make: $(make --version | head -n1)" || print_error "Make no encontrado"
    command -v git &> /dev/null && print_step "Git: $(git --version)" || print_error "Git no encontrado"

    echo ""
}

show_instructions() {
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║          INSTALACIÓN COMPLETADA EXITOSAMENTE            ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "Para compilar y flashear:"
    echo -e "  ${YELLOW}cd /mnt/disk/src/rpico/pico_src/pendrive${NC}"
    echo -e "  ${YELLOW}bash flash_pendrive.sh${NC}"
    echo ""
    echo -e "Para solo compilar:"
    echo -e "  ${YELLOW}bash flash_pendrive.sh --compile-only${NC}"
    echo ""
    echo -e "Para solo flashear:"
    echo -e "  ${YELLOW}bash flash_pendrive.sh --flash-only${NC}"
    echo ""
}

#==============================================================================
# MAIN
#==============================================================================

print_header
check_root
update_repos
install_build_tools
install_arm_compiler
verify_picotool
verify_install
show_instructions

echo ""
print_step "Script de flasheo listo: flash_pendrive.sh"
echo ""
