#!/bin/bash
#==============================================================================
# check_midi.sh - Verificar estado del dispositivo MIDI
#==============================================================================
# Uso: bash check_midi.sh
#==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo ""
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║          VERIFICADOR DE DISPOSITIVO MIDI                ║${NC}"
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

print_header

# 1. Verificar dispositivos USB
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                    DISPOSITIVOS USB                      ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

if command -v lsusb &> /dev/null; then
    lsusb
else
    print_warning "lsusb no disponible. Instale usbutils:"
    echo "  sudo apt install usbutils"
fi

echo ""

# 2. Buscar Pico
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                    BUSCANDO PICO                         ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

if lsusb | grep -q "cafe:4015"; then
    print_step "Raspberry Pi Pico detectado como MIDI USB"
    echo ""
    echo "  Vendor ID:  0xCafe"
    echo "  Product ID: 0x4015"
    echo "  Nombre:     Pico Touch MIDI"
    echo ""
    
    # Extraer información completa
    PICO_LINE=$(lsusb | grep "cafe:4015")
    print_info "Detalles: $PICO_LINE"
    
elif lsusb | grep -q "2e8a:0003"; then
    print_warning "Raspberry Pi Pico detectado en modo BOOTSEL"
    echo ""
    echo "  El Pico está listo para flashear"
    echo "  Ejecute: bash script_tools/flash.sh"
    
elif lsusb | grep -q "2e8a"; then
    print_warning "Raspberry Pi Pico detectado (otro modo)"
    lsusb | grep "2e8a"
    
else
    print_error "Raspberry Pi Pico no detectado"
    echo ""
    echo "  Posibles causas:"
    echo "  - El Pico no está conectado"
    echo "  - El cable USB no funciona"
    echo "  - El Pico está apagado"
fi

echo ""

# 3. Verificar dispositivos de sonido
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                 DISPOSITIVOS DE SONIDO                   ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

if command -v arecord &> /dev/null; then
    echo "Dispositivos de audio:"
    arecord -l 2>/dev/null || echo "  No hay dispositivos de audio"
else
    print_warning "arecord no disponible"
fi

echo ""

# 4. Verificar dispositivos MIDI (ALSA)
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                    DISPOSITIVOS MIDI                     ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

if [[ -d /dev/snd ]]; then
    print_step "Subsistema de sonido disponible"
    echo ""
    echo "Dispositivos MIDI:"
    ls /dev/snd/ 2>/dev/null | grep midi || echo "  No hay dispositivos MIDI"
else
    print_warning "Subsistema de sonido no disponible"
fi

echo ""

# 5. Verificar dmesg
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                  ÚLTIMOS MENSAJES USB                    ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

print_info "Últimos 10 mensajes USB del kernel:"
dmesg | grep -i usb | tail -10 2>/dev/null || echo "  (no se pudo acceder a dmesg)"

echo ""

# 6. Verificar si hay procesos MIDI
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                PROCESOS MIDI ACTIVOS                     ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

MIDI_PROCS=$(ps aux | grep -i midi | grep -v grep | grep -v "check_midi")
if [[ -n "$MIDI_PROCS" ]]; then
    print_step "Procesos MIDI activos:"
    echo "$MIDI_PROCS"
else
    print_info "No hay procesos MIDI activos"
fi

echo ""

# Resumen
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                       RESUMEN                            ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

if lsusb | grep -q "cafe:4015"; then
    echo -e "${GREEN}✅ El Pico está conectado y funcionando como MIDI USB${NC}"
    echo ""
    echo "  Para usarlo:"
    echo "  - Abrir un sintetizador MIDI (SimpleSynth, FL Studio, etc.)"
    echo "  - Seleccionar 'Pico Touch MIDI' como input"
    echo "  - Tocar los pads!"
else
    echo -e "${YELLOW}⚠️  El Pico no está funcionando como MIDI USB${NC}"
    echo ""
    echo "  Pasos para solucionar:"
    echo "  1. Verificar conexión USB"
    echo "  2. Reiniciar el Pico"
    echo "  3. Re-flashear el firmware"
fi

echo ""
