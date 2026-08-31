#!/bin/bash
#==============================================================================
# flash_adc_oled.sh - Compilar y flashear firmware ADC+OLED al Raspberry Pi Pico
#==============================================================================
# Uso: bash flash_adc_oled.sh [--compile-only] [--flash-only]
#==============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_DIR/build"
FIRMWARE="$BUILD_DIR/src/adc_oled.uf2"
PICOTOOL="/mnt/disk/src/rpico/picotool/build/picotool"

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
    echo -e "${BLUE}║     FLASH ADC+OLED - RASPBERRY PI PICO RP2040          ║${NC}"
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

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

compile_project() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}                    PASO 1: COMPILAR                      ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    
    cd "$PROJECT_DIR"
    
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
    
    if [[ -f "$FIRMWARE" ]]; then
        SIZE=$(ls -lh "$FIRMWARE" | awk '{print $5}')
        print_step "Firmware generado: adc_oled.uf2 ($SIZE)"
    else
        print_error "Firmware no generado"
        exit 1
    fi
}

flash_firmware() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}                    PASO 2: FLASHEAR                      ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    
    if [[ ! -f "$FIRMWARE" ]]; then
        print_error "Firmware no encontrado: $FIRMWARE"
        print_info "Primero compile el proyecto"
        exit 1
    fi
    
    if [[ -x "$PICOTOOL" ]]; then
        print_info "Usando picotool local: $PICOTOOL"
        
        if lsusb | grep -q "2e8a:0003"; then
            print_step "Pico en modo BOOTSEL detectado"
            "$PICOTOOL" load "$FIRMWARE" -f
            print_step "Firmware cargado con picotool"
        else
            print_info "Intentando reiniciar Pico en modo BOOTSEL..."
            "$PICOTOOL" reboot -u 2>/dev/null || true
            sleep 3
            
            if lsusb | grep -q "2e8a:0003"; then
                "$PICOTOOL" load "$FIRMWARE" -f
                print_step "Firmware cargado con picotool"
            else
                print_warning "No se pudo reiniciar el Pico automáticamente"
                echo ""
                echo "  Para entrar en modo BOOTSEL manualmente:"
                echo "  1. DESCONECTAR el Pico del USB"
                echo "  2. Mantener presionado el botón BOOTSEL"
                echo "  3. CONECTAR el Pico al USB mientras mantienes BOOTSEL"
                echo "  4. SOLTAR el botón cuando la LED se encienda"
                echo ""
                read -p "Presione Enter cuando el Pico esté en modo BOOTSEL..."
                
                if lsusb | grep -q "2e8a:0003"; then
                    "$PICOTOOL" load "$FIRMWARE" -f
                    print_step "Firmware cargado con picotool"
                else
                    print_error "Pico no detectado en modo BOOTSEL"
                    exit 1
                fi
            fi
        fi
        
        # Try to reboot after loading
        print_info "Reiniciando Pico..."
        "$PICOTOOL" reboot 2>/dev/null || true
        sleep 2
        
    elif command -v picotool &> /dev/null; then
        print_info "Usando picotool del sistema..."
        
        if lsusb | grep -q "2e8a:0003"; then
            picotool load "$FIRMWARE" -f
        else
            picotool reboot -u 2>/dev/null || true
            sleep 3
            picotool load "$FIRMWARE" -f
        fi
        
        print_info "Reiniciando Pico..."
        picotool reboot 2>/dev/null || true
        sleep 2
    else
        print_error "picotool no encontrado"
        print_info "Compila picotool desde /mnt/disk/src/rpico/picotool"
        exit 1
    fi
}

verify_device() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}                    PASO 3: VERIFICAR                     ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    
    print_info "Esperando reinicio del Pico..."
    sleep 3
    
    # Check if Pico is in BOOTSEL mode
    if lsusb | grep -q "2e8a:0003"; then
        print_warning "El Pico aún está en modo BOOTSEL"
        print_info "Intentando reiniciar..."
        "$PICOTOOL" reboot 2>/dev/null || true
        sleep 2
    fi
    
    # Check if CDC device appeared
    if lsusb | grep -q "cafe:4016"; then
        echo ""
        echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}║           ¡FIRMWARE CARGADO EXITOSAMENTE!              ║${NC}"
        echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
        echo ""
        print_step "Dispositivo CDC detectado (cafe:4016)"
        print_info "Verifique en el OLED que aparezca la version: "FIRMWARE_VERSION""
        print_info "Puerto serial: /dev/ttyACM0"
        echo ""
        echo "Para ver datos:"
        echo "  minicom -D /dev/ttyACM0 -b 115200"
    else
        print_warning "Dispositivo CDC no detectado aún"
        print_info "El Pico puede estar ejecutando el firmware sin CDC"
        print_info "Verifique el OLED para confirmar"
        echo ""
        echo "Si no aparece el OLED:"
        echo "  1. Verifique conexiones I2C (GP16=SDA, GP17=SCL)"
        echo "  2. Verifique alimentacion 3.3V y GND"
        echo "  3. Verifique direccion I2C (0x3C o 0x3D)"
    fi
}

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
