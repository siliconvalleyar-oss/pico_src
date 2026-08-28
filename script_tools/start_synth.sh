#!/bin/bash
#==============================================================================
# start_synth.sh - Iniciar FluidSynth y conectar al Pico MIDI
#==============================================================================
# Uso: bash start_synth.sh
#==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo ""
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║           SYNTH MIDI - RASPBERRY PI PICO               ║${NC}"
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

# Verificar Pico
check_pico() {
    print_info "Verificando Pico MIDI..."
    
    if aconnect -l 2>/dev/null | grep -q "Pico Touch MIDI"; then
        print_step "Pico Touch MIDI detectado"
        return 0
    else
        print_error "Pico MIDI no detectado"
        echo "  Verifique: lsusb | grep cafe"
        return 1
    fi
}

# Verificar SoundFont
check_soundfont() {
    SOUNDFONT="/usr/share/sounds/sf2/FluidR3_GM.sf2"
    
    if [[ -f "$SOUNDFONT" ]]; then
        print_step "SoundFont encontrado: $SOUNDFONT"
        return 0
    else
        print_error "SoundFont no encontrado"
        echo "  Instale: sudo apt install fluid-soundfont-gm"
        return 1
    fi
}

# Obtener device ID del Pico
get_pico_device() {
    # Buscar el device number del Pico
    DEVICE_INFO=$(aconnect -l 2>/dev/null | grep -A1 "Pico Touch MIDI" | grep "client" | head -1)
    if [[ -n "$DEVICE_INFO" ]]; then
        # Extraer número del device
        PICO_CLIENT=$(echo "$DEVICE_INFO" | grep -oP '\d+(?=:)')
        echo "$PICO_CLIENT"
    else
        echo ""
    fi
}

# Iniciar FluidSynth
start_fluidsynth() {
    SOUNDFONT="/usr/share/sounds/sf2/FluidR3_GM.sf2"
    
    print_info "Iniciando FluidSynth..."
    
    # Matar instancias anteriores
    pkill fluidsynth 2>/dev/null
    sleep 1
    
    # Iniciar FluidSynth en background
    fluidsynth -a alsa -m alsa_seq -g 1.0 "$SOUNDFONT" &
    FLUIDSYNTH_PID=$!
    
    sleep 2
    
    if ps -p $FLUIDSYNTH_PID > /dev/null; then
        print_step "FluidSynth iniciado (PID: $FLUIDSYNTH_PID)"
        return 0
    else
        print_error "Error al iniciar FluidSynth"
        return 1
    fi
}

# Conectar Pico a FluidSynth
connect_midi() {
    print_info "Conectando Pico a FluidSynth..."
    
    # Obtener clientes
    PICO_CLIENT=$(aconnect -l 2>/dev/null | grep -B1 "Pico Touch MIDI" | grep "client" | grep -oP '\d+')
    FLUID_CLIENT=$(aconnect -l 2>/dev/null | grep -B1 "FLUID Synth" | grep "client" | grep -oP '\d+')
    
    if [[ -z "$PICO_CLIENT" ]]; then
        print_error "No se encontró el cliente Pico"
        return 1
    fi
    
    if [[ -z "$FLUID_CLIENT" ]]; then
        print_error "No se encontró FluidSynth"
        echo "  Ejecute: bash start_synth.sh"
        return 1
    fi
    
    print_info "Pico client: $PICO_CLIENT, FluidSynth client: $FLUID_CLIENT"
    
    # Conectar
    aconnect $PICO_CLIENT:0 $FLUID_CLIENT:0
    
    if [[ $? -eq 0 ]]; then
        print_step "Conexión establecida"
        return 0
    else
        print_error "Error al conectar"
        return 1
    fi
}

# Mostrar info
show_info() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}                    SINTH LISTO                          ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    echo -e "  ${GREEN}✓ FluidSynth está ejecutándose${NC}"
    echo -e "  ${GREEN}✓ Conectado al Pico MIDI${NC}"
    echo -e "  ${GREEN}✓ SoundFont: General MIDI${NC}"
    echo ""
    echo -e "  ${YELLOW}Toca los pads del Pico para escuchar sonidos!${NC}"
    echo ""
    echo -e "  Para cambiar instrumento:"
    echo -e "    ${BLUE}aconnect -l${NC}                    # Ver conexiones"
    echo -e "    ${BLUE}amidi -p hw:4 -S 'C0 0'${NC}      # Cambiar a Piano"
    echo -e "    ${BLUE}amidi -p hw:4 -S 'C0 25'${NC}     # Cambiar a Guitar"
    echo -e "    ${BLUE}amidi -p hw:4 -S 'C0 40'${NC}     # Cambiar a Violín"
    echo ""
    echo -e "  Para salir: ${RED}Ctrl+C o kill $FLUIDSYNTH_PID${NC}"
    echo ""
}

#══════════════════════════════════════════════════════════════
# MAIN
#══════════════════════════════════════════════════════════════

print_header

# Verificar dependencias
check_pico || exit 1
check_soundfont || exit 1

# Iniciar synth
start_fluidsynth || exit 1

# Conectar
connect_midi || exit 1

# Mostrar info
show_info

# Mantener script corriendo
echo -e "${BLUE}Presione Ctrl+C para detener${NC}"
echo ""

# Esperar a que termine FluidSynth
wait $FLUIDSYNTH_PID 2>/dev/null
