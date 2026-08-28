#!/bin/bash
#==============================================================================
# fix_first_build.sh - Corregir problemas comunes en la primera compilación
#==============================================================================
# Uso: bash fix_first_build.sh
#==============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_DIR="/home/joy/src/pico_src/midi"
SDK_DIR="/home/joy/src/pico/pico-sdk"

print_header() {
    echo ""
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║       SOLUCIONADOR DE PROBLEMAS - PRIMER BUILD          ║${NC}"
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

# Verificar y crear directorios necesarios
fix_directories() {
    print_step "Verificando estructura de directorios..."
    
    # Crear directorio del proyecto si no existe
    if [[ ! -d "$PROJECT_DIR" ]]; then
        print_error "Directorio del proyecto no encontrado: $PROJECT_DIR"
        exit 1
    fi
    
    # Crear directorio build si no existe
    if [[ ! -d "$PROJECT_DIR/build" ]]; then
        mkdir -p "$PROJECT_DIR/build"
        print_step "Directorio build creado"
    fi
    
    # Verificar que existen los archivos fuente
    REQUIRED_FILES=(
        "src/main.c"
        "src/tusb_config.h"
        "src/usb_descriptors.c"
        "src/CMakeLists.txt"
        "CMakeLists.txt"
        "pico_sdk_import.cmake"
    )
    
    for file in "${REQUIRED_FILES[@]}"; do
        if [[ -f "$PROJECT_DIR/$file" ]]; then
            print_step "Archivo encontrado: $file"
        else
            print_error "Archivo faltante: $file"
            exit 1
        fi
    done
}

# Verificar y corregir permisos
fix_permissions() {
    print_step "Verificando permisos..."
    
    # Dar permisos de ejecución a scripts
    chmod +x "$PROJECT_DIR"/script_tools/*.sh 2>/dev/null || true
    
    # Verificar permisos del SDK
    if [[ -d "$SDK_DIR" ]]; then
        # Asegurar que joy puede acceder al SDK
        chown -R joy:joy "$SDK_DIR" 2>/dev/null || true
        print_step "Permisos del SDK corregidos"
    fi
    
    # Verificar permisos del proyecto
    chown -R joy:joy "$PROJECT_DIR" 2>/dev/null || true
    print_step "Permisos del proyecto corregidos"
}

# Verificar y configurar el SDK
fix_sdk() {
    print_step "Verificando Pico SDK..."
    
    if [[ ! -d "$SDK_DIR" ]]; then
        print_warning "Pico SDK no encontrado. Clonando..."
        mkdir -p /home/joy/src/pico
        cd /home/joy/src/pico
        git clone https://github.com/raspberrypi/pico-sdk.git
        cd pico-sdk
        git submodule update --init --recursive
        chown -R joy:joy /home/joy/src/pico
        print_step "Pico SDK clonado"
    fi
    
    # Verificar submodulos
    cd "$SDK_DIR"
    print_info "Verificando submodulos del SDK..."
    
    # TinyUSB es esencial para MIDI
    if [[ ! -d "$SDK_DIR/lib/tinyusb" ]]; then
        print_warning "TinyUSB no encontrado. Inicializando submodule..."
        git submodule update --init lib/tinyusb
    fi
    
    if [[ -d "$SDK_DIR/lib/tinyusb/src" ]]; then
        print_step "TinyUSB verificado"
    else
        print_error "TinyUSB no se pudo inicializar"
        exit 1
    fi
    
    # BTstack (opcional pero recomendado)
    if [[ ! -d "$SDK_DIR/lib/btstack" ]]; then
        print_info "Inicializando BTstack submodule..."
        git submodule update --init lib/btstack
    fi
    
    # CYW43 driver (para Pico W)
    if [[ ! -d "$SDK_DIR/lib/cyw43-driver" ]]; then
        print_info "Inicializando CYW43 driver..."
        git submodule update --init lib/cyw43-driver
    fi
    
    print_step "SDK verificado correctamente"
}

# Limpiar builds anteriores
clean_build() {
    print_step "Limpiando builds anteriores..."
    
    BUILD_DIR="$PROJECT_DIR/build"
    
    if [[ -d "$BUILD_DIR" ]]; then
        # Verificar si hay errores en CMakeCache
        if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
            if grep -q "PICO_SDK_PATH" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
                print_info "Limpiando cache de CMake..."
                rm -rf "$BUILD_DIR"
                mkdir -p "$BUILD_DIR"
                print_step "Build limpio creado"
            fi
        fi
    fi
}

# Configurar CMakeLists.txt con el path correcto del SDK
fix_cmake_path() {
    print_step "Verificando path del SDK en CMakeLists.txt..."
    
    CMAKE_FILE="$PROJECT_DIR/CMakeLists.txt"
    
    if [[ ! -f "$CMAKE_FILE" ]]; then
        print_error "CMakeLists.txt no encontrado"
        exit 1
    fi
    
    # Verificar si el path del SDK está configurado
    if grep -q "pico-sdk" "$CMAKE_FILE"; then
        print_step "Path del SDK configurado en CMakeLists.txt"
    else
        print_warning "Path del SDK no encontrado. Configurando..."
        
        # Crear CMakeLists.txt con el path correcto
        cat > "$CMAKE_FILE" << 'EOF'
cmake_minimum_required(VERSION 3.13)

# Set PICO_SDK_PATH to the SDK directory
set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../pico/pico-sdk")

# Pull in Pico SDK (must be done before project())
include(pico_sdk_import.cmake)

project(midi_touch_pads C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Add source subdirectory
add_subdirectory(src)
EOF
        print_step "CMakeLists.txt configurado"
    fi
}

# Compilar el proyecto
build_project() {
    print_step "Compilando proyecto..."
    
    cd "$PROJECT_DIR"
    
    # Limpiar build anterior si existe
    rm -rf build
    mkdir -p build
    cd build
    
    print_info "Ejecutando CMake..."
    if cmake .. 2>&1; then
        print_step "CMake configurado correctamente"
    else
        print_error "Error en CMake"
        exit 1
    fi
    
    print_info "Compilando con make..."
    if make -j$(nproc) 2>&1; then
        print_step "Compilación exitosa"
    else
        print_error "Error en la compilación"
        exit 1
    fi
    
    # Verificar que se generó el .uf2
    if [[ -f "src/midi.uf2" ]]; then
        SIZE=$(ls -lh src/midi.uf2 | awk '{print $5}')
        print_step "Firmware generado: src/midi.uf2 ($SIZE)"
    else
        print_error "Archivo .uf2 no generado"
        exit 1
    fi
}

# Verificar dispositivo USB
check_usb_device() {
    print_step "Verificando dispositivos USB..."
    
    echo ""
    print_info "Dispositivos USB conectados:"
    lsusb 2>/dev/null || echo "  (lsusb no disponible)"
    
    echo ""
    print_info "Buscando Raspberry Pi Pico..."
    
    if lsusb | grep -q "cafe:4015"; then
        print_step "Raspberry Pi Pico detectado como MIDI"
    elif lsusb | grep -q "2e8a:0003"; then
        print_warning "Raspberry Pi Pico detectado en modo BOOTSEL"
        print_info "El Pico está listo para flashear"
    elif lsusb | grep -q "2e8a"; then
        print_warning "Raspberry Pi Pico detectado (otro modo)"
    else
        print_warning "Raspberry Pi Pico no detectado"
        print_info "Asegúrese de que esté conectado por USB"
    fi
}

# Mostrar instrucciones
show_instructions() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}                    INSTRUCCIONES                         ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    echo -e "${GREEN}Para flashear el Pico:${NC}"
    echo "  1. Mantener presionado el botón BOOTSEL en el Pico"
    echo "  2. Conectar el Pico al USB de la Raspberry"
    echo "  3. Soltar el botón BOOTSEL"
    echo "  4. Ejecutar:"
    echo ""
    echo -e "    ${YELLOW}sudo cp /home/joy/src/pico_src/midi/build/src/midi.uf2 /media/joy/RPI-RP2/${NC}"
    echo ""
    echo -e "${GREEN}Para verificar el dispositivo MIDI:${NC}"
    echo -e "    ${YELLOW}lsusb | grep -i cafe${NC}"
    echo ""
    echo -e "${GREEN}Para volver a compilar:${NC}"
    echo -e "    ${YELLOW}cd /home/joy/src/pico_src/midi/build && make -j4${NC}"
    echo ""
}

#══════════════════════════════════════════════════════════════
# MAIN
#══════════════════════════════════════════════════════════════

print_header
fix_directories
fix_permissions
fix_sdk
clean_build
fix_cmake_path
build_project
check_usb_device
show_instructions

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║           ¡TODO LISTO PARA FLASHEAR EL PICO!           ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""
