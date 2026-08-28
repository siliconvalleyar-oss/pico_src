#!/bin/bash
#==============================================================================
# install_deps.sh - Instalar dependencias para compilar proyectos Pico
#==============================================================================
# Uso: sudo bash install_deps.sh
#==============================================================================

set -e  # Salir si hay error

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo ""
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║     INSTALADOR DE DEPENDENCIAS - RASPBERRY PI PICO     ║${NC}"
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

# Verificar si se ejecuta como root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "Este script debe ejecutarse como root"
        echo "Uso: sudo bash $0"
        exit 1
    fi
}

# Detectar sistema operativo
detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        OS=$ID
        OS_VERSION=$VERSION_ID
    else
        print_error "No se pudo detectar el sistema operativo"
        exit 1
    fi
    print_info "Sistema detectado: $OS $OS_VERSION"
}

# Actualizar repositorios
update_repos() {
    print_step "Actualizando repositorios..."
    apt-get update -qq
}

# Instalar compilador ARM
install_arm_compiler() {
    print_step "Instalando compilador ARM (arm-none-eabi-gcc)..."
    
    if command -v arm-none-eabi-gcc &> /dev/null; then
        INSTALLED_VERSION=$(arm-none-eabi-gcc --version | head -n1)
        print_warning "Ya instalado: $INSTALLED_VERSION"
    else
        apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi
        print_step "Compilador ARM instalado"
    fi
}

# Instalar herramientas de build
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
    print_step "Herramientas de build instaladas"
}

# Instalar dependencias adicionales
install_extras() {
    print_step "Instalando dependencias adicionales..."
    apt-get install -y \
        wget \
        curl \
        unzip \
        sudo \
        nano \
        htop \
        lsusb \
        usbutils
    print_step "Dependencias adicionales instaladas"
}

# Clonar Pico SDK si no existe
install_pico_sdk() {
    SDK_DIR="/home/joy/src/pico/pico-sdk"
    
    if [[ -d "$SDK_DIR" ]]; then
        print_warning "Pico SDK ya existe en $SDK_DIR"
        print_info "Actualizando submodulos..."
        cd "$SDK_DIR"
        git submodule update --init --recursive
    else
        print_step "Clonando Pico SDK..."
        mkdir -p /home/joy/src/pico
        cd /home/joy/src/pico
        git clone https://github.com/raspberrypi/pico-sdk.git
        cd pico-sdk
        git submodule update --init --recursive
        chown -R joy:joy /home/joy/src/pico
    fi
    print_step "Pico SDK listo"
}

# Instalar picotool (opcional)
install_picotool() {
    print_step "Verificando picotool..."
    
    if command -v picotool &> /dev/null; then
        print_warning "picotool ya instalado"
    else
        print_info "picotool se compilará automáticamente en el primer build"
    fi
}

# Verificar instalación
verify_install() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    print_step "Verificando instalación..."
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    
    # Verificar compilador
    if command -v arm-none-eabi-gcc &> /dev/null; then
        VERSION=$(arm-none-eabi-gcc --version | head -n1)
        print_step "Compilador ARM: $VERSION"
    else
        print_error "Compilador ARM no encontrado"
    fi
    
    # Verificar cmake
    if command -v cmake &> /dev/null; then
        VERSION=$(cmake --version | head -n1)
        print_step "CMake: $VERSION"
    else
        print_error "CMake no encontrado"
    fi
    
    # Verificar make
    if command -v make &> /dev/null; then
        VERSION=$(make --version | head -n1)
        print_step "Make: $VERSION"
    else
        print_error "Make no encontrado"
    fi
    
    # Verificar git
    if command -v git &> /dev/null; then
        VERSION=$(git --version)
        print_step "Git: $VERSION"
    else
        print_error "Git no encontrado"
    fi
    
    # Verificar Python
    if command -v python3 &> /dev/null; then
        VERSION=$(python3 --version)
        print_step "Python: $VERSION"
    else
        print_error "Python3 no encontrado"
    fi
    
    # Verificar SDK
    SDK_DIR="/home/joy/src/pico/pico-sdk"
    if [[ -d "$SDK_DIR" ]]; then
        print_step "Pico SDK: $SDK_DIR"
    else
        print_error "Pico SDK no encontrado"
    fi
    
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║          INSTALACIÓN COMPLETADA EXITOSAMENTE           ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

#══════════════════════════════════════════════════════════════
# MAIN
#══════════════════════════════════════════════════════════════

print_header
check_root
detect_os
update_repos
install_arm_compiler
install_build_tools
install_extras
install_pico_sdk
install_picotool
verify_install

echo ""
print_step "Para compilar el proyecto MIDI:"
echo "  cd /home/joy/src/pico_src/midi"
echo "  bash script_tools/fix_first_build.sh"
echo ""
