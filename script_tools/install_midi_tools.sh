#!/bin/bash
#==============================================================================
# install_midi_tools.sh - Instalar todas las herramientas MIDI para Pico
#==============================================================================
# Uso: sudo bash install_midi_tools.sh
#==============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

print_header() {
    echo ""
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║     INSTALADOR DE HERRAMIENTAS MIDI - RASPBERRY PI     ║${NC}"
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

print_section() {
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  $1${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    echo ""
}

# Check root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "Este script debe ejecutarse como root"
        echo "Uso: sudo bash $0"
        exit 1
    fi
}

# Update repos
update_repos() {
    print_section "ACTUALIZANDO REPOSITORIOS"
    apt-get update -qq
    print_step "Repositorios actualizados"
}

# Install build tools
install_build_tools() {
    print_section "HERRAMIENTAS DE COMPILACIÓN"
    
    apt-get install -y \
        build-essential \
        cmake \
        git \
        wget \
        curl \
        unzip \
        pkg-config
    
    print_step "Build tools instalados"
}

# Install ARM compiler
install_arm_compiler() {
    print_section "COMPILADOR ARM (Pico SDK)"
    
    if command -v arm-none-eabi-gcc &> /dev/null; then
        VERSION=$(arm-none-eabi-gcc --version | head -n1)
        print_warning "Ya instalado: $VERSION"
    else
        apt-get install -y \
            gcc-arm-none-eabi \
            libnewlib-arm-none-eabi
        print_step "Compilador ARM instalado"
    fi
}

# Install audio tools
install_audio_tools() {
    print_section "HERRAMIENTAS DE AUDIO"
    
    apt-get install -y \
        fluidsynth \
        fluid-soundfont-gm \
        alsa-utils \
        pulseaudio \
        pulseaudio-utils \
        bluez \
        bluez-tools
    
    print_step "Herramientas de audio instaladas"
}

# Install MIDI tools
install_midi_tools() {
    print_section "HERRAMIENTAS MIDI"
    
    apt-get install -y \
        libasound2-dev \
        libasound2-plugins
    
    print_step "ALSA MIDI instalado"
}

# Install Python tools
install_python_tools() {
    print_section "HERRAMIENTAS PYTHON"
    
    apt-get install -y \
        python3 \
        python3-pip \
        python3-dev
    
    # Install Python MIDI libraries
    pip3 install --break-system-packages mido python-rtmidi 2>/dev/null || \
    pip3 install mido python-rtmidi
    
    print_step "Python MIDI libraries instaladas"
}

# Install picotool
install_picotool() {
    print_section "PICOTOOL"
    
    PICOTOOL_DIR="/home/joy/src/pico/picotool"
    
    if [[ -f "$PICOTOOL_DIR/build/picotool" ]]; then
        print_warning "picotool ya compilado en $PICOTOOL_DIR"
    else
        print_info "picotool se compilará con el SDK"
    fi
}

# Install Pico SDK
install_pico_sdk() {
    print_section "PICO SDK"
    
    SDK_DIR="/home/joy/src/pico/pico-sdk"
    
    if [[ -d "$SDK_DIR" ]]; then
        print_warning "Pico SDK ya existe en $SDK_DIR"
        print_info "Actualizando submodulos..."
        cd "$SDK_DIR"
        git submodule update --init --recursive 2>/dev/null || true
    else
        print_info "Clonando Pico SDK..."
        mkdir -p /home/joy/src/pico
        cd /home/joy/src/pico
        git clone https://github.com/raspberrypi/pico-sdk.git
        cd pico-sdk
        git submodule update --init --recursive
        chown -R joy:joy /home/joy/src/pico
    fi
    
    print_step "Pico SDK listo"
}

# Install Bluetooth tools
install_bluetooth_tools() {
    print_section "HERRAMIENTAS BLUETOOTH"
    
    apt-get install -y \
        bluetooth \
        bluez \
        bluez-tools \
        pulseaudio-module-bluetooth
    
    # Restart bluetooth service
    systemctl restart bluetooth 2>/dev/null || true
    
    print_step "Herramientas Bluetooth instaladas"
}

# Create project structure
setup_project() {
    print_section "CONFIGURAR PROYECTO"
    
    PROJECT_DIR="/home/joy/src/pico_src"
    
    if [[ -d "$PROJECT_DIR" ]]; then
        print_warning "Proyecto ya existe en $PROJECT_DIR"
        cd "$PROJECT_DIR"
        git pull 2>/dev/null || true
    else
        print_info "Clonando proyecto..."
        cd /home/joy/src
        git clone https://github.com/siliconvalleyar-oss/pico_src.git
        chown -R joy:joy "$PROJECT_DIR"
    fi
    
    print_step "Proyecto configurado"
}

# Create quick start script
create_quickstart() {
    print_section "CREAR SCRIPT DE INICIO RÁPIDO"
    
    cat > /home/joy/start_midi.sh << 'EOF'
#!/bin/bash
# start_midi.sh - Iniciar sistema MIDI completo

echo "Iniciando FluidSynth..."
pkill -9 fluidsynth 2>/dev/null
sleep 1

# Start FluidSynth
python3 /home/joy/src/pico_src/script_tools/start_bt_synth.py &
sleep 5

# Connect MIDI
FLUID_CLIENT=$(aconnect -l | grep -B1 'FLUID Synth' | grep 'client' | grep -oP '\d+' | head -1)
if [ -n "$FLUID_CLIENT" ]; then
    aconnect 32:0 $FLUID_CLIENT:0 2>/dev/null
    echo "MIDI conectado!"
fi

echo ""
echo "════════════════════════════════════════════"
echo "  ¡SISTEMA MIDI LISTO!"
echo "  Toca los pads del Pico"
echo "════════════════════════════════════════════"
EOF
    
    chmod +x /home/joy/start_midi.sh
    chown joy:joy /home/joy/start_midi.sh
    
    print_step "Script de inicio creado: /home/joy/start_midi.sh"
}

# Verify installation
verify_install() {
    print_section "VERIFICAR INSTALACIÓN"
    
    echo -e "${BLUE}Herramientas instaladas:${NC}"
    echo ""
    
    # Build tools
    command -v gcc &> /dev/null && print_step "gcc: $(gcc --version | head -n1)" || print_error "gcc no encontrado"
    command -v cmake &> /dev/null && print_step "cmake: $(cmake --version | head -n1)" || print_error "cmake no encontrado"
    command -v make &> /dev/null && print_step "make: $(make --version | head -n1)" || print_error "make no encontrado"
    command -v git &> /dev/null && print_step "git: $(git --version)" || print_error "git no encontrado"
    
    # ARM compiler
    command -v arm-none-eabi-gcc &> /dev/null && print_step "ARM GCC: $(arm-none-eabi-gcc --version | head -n1)" || print_error "ARM GCC no encontrado"
    
    # Audio
    command -v fluidsynth &> /dev/null && print_step "FluidSynth: $(fluidsynth --version | head -n1)" || print_error "FluidSynth no encontrado"
    command -v pactl &> /dev/null && print_step "PulseAudio: $(pactl --version | head -n1)" || print_error "PulseAudio no encontrado"
    
    # Python
    command -v python3 &> /dev/null && print_step "Python: $(python3 --version)" || print_error "Python no encontrado"
    python3 -c "import mido" 2>/dev/null && print_step "mido: instalado" || print_error "mido no encontrado"
    python3 -c "import rtmidi" 2>/dev/null && print_step "python-rtmidi: instalado" || print_error "python-rtmidi no encontrado"
    
    # SoundFont
    [[ -f "/usr/share/sounds/sf2/FluidR3_GM.sf2" ]] && print_step "SoundFont: FluidR3_GM.sf2" || print_error "SoundFont no encontrado"
    
    # Pico SDK
    [[ -d "/home/joy/src/pico/pico-sdk" ]] && print_step "Pico SDK: instalado" || print_error "Pico SDK no encontrado"
    
    # Project
    [[ -d "/home/joy/src/pico_src" ]] && print_step "Proyecto: pico_src" || print_error "Proyecto no encontrado"
    
    echo ""
}

# Show final instructions
show_instructions() {
    print_section "INSTRUCCIONES FINALES"
    
    echo -e "${GREEN}¡Instalación completada!${NC}"
    echo ""
    echo -e "Para iniciar el sistema MIDI:"
    echo -e "  ${YELLOW}bash /home/joy/start_midi.sh${NC}"
    echo ""
    echo -e "O manualmente:"
    echo -e "  ${YELLOW}cd /home/joy/src/pico_src${NC}"
    echo -e "  ${YELLOW}python3 script_tools/start_bt_synth.py${NC}"
    echo ""
    echo -e "Para compilar el firmware:"
    echo -e "  ${YELLOW}bash script_tools/flash_full.sh${NC}"
    echo ""
    echo -e "Para cambiar instrumento:"
    echo -e "  ${YELLOW}python3 -c 'import mido; p=mido.open_output([x for x in mido.get_output_names() if \"FLUID\" in x][0]); p.send(mido.Message(\"program_change\", program=73, channel=0))'${NC}"
    echo ""
}

#══════════════════════════════════════════════════════════════
# MAIN
#══════════════════════════════════════════════════════════════

print_header
check_root
update_repos
install_build_tools
install_arm_compiler
install_audio_tools
install_midi_tools
install_python_tools
install_picotool
install_pico_sdk
install_bluetooth_tools
setup_project
create_quickstart
verify_install
show_instructions

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║         INSTALACIÓN COMPLETADA EXITOSAMENTE            ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""
