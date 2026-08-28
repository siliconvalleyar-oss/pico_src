#!/bin/bash
#==============================================================================
# change_instrument.sh - Cambiar instrumento MIDI en FluidSynth
#==============================================================================
# Uso: bash change_instrument.sh [instrumento]
#==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

print_header() {
    echo ""
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║          CAMBIAR INSTRUMENTO MIDI                      ║${NC}"
    echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

# Instrumentos GM (General MIDI)
declare -A INSTRUMENTS=(
    # Piano
    ["piano"]=0 ["acoustic_grand"]=0 ["bright_piano"]=1 ["electric_grand"]=2
    ["honky_tonk"]=3 ["electric_piano"]=4 ["electric_piano2"]=5 ["harpsichord"]=6
    
    # Chromatic
    ["clavinet"]=7 ["celesta"]=8 ["glockenspiel"]=9 ["music_box"]=10
    ["vibraphone"]=11 ["marimba"]=12 ["xylophone"]=13 ["tubular_bells"]=14
    ["dulcimer"]=15
    
    # Organ
    ["drawbar_organ"]=16 ["percussive_organ"]=17 ["rock_organ"]=18
    ["church_organ"]=19 ["reed_organ"]=20 ["accordion"]=21 ["harmonica"]=22
    ["tango_accordion"]=23
    
    # Guitar
    ["nylon_guitar"]=24 ["steel_guitar"]=25 ["jazz_guitar"]=26
    ["clean_guitar"]=27 ["muted_guitar"]=28 ["overdriven_guitar"]=29
    ["distortion_guitar"]=30 ["guitar_harmonics"]=31
    
    # Bass
    ["acoustic_bass"]=32 ["finger_bass"]=33 ["pick_bass"]=34
    ["fretless_bass"]=35 ["slap_bass"]=36 ["synth_bass"]=37 ["synth_bass2"]=38
    
    # Strings
    ["violin"]=40 ["viola"]=41 ["cello"]=42 ["contrabass"]=43
    ["tremolo_strings"]=44 ["pizzicato_strings"]=45
    ["harp"]=46 ["timpani"]=47
    
    # Ensemble
    ["string_ensemble"]=48 ["string_ensemble2"]=49
    ["synth_strings"]=50 ["synth_strings2"]=51
    ["choir_aahs"]=52 ["voice_oohs"]=53 ["synth_choir"]=54
    ["orchestra_hit"]=55
    
    # Brass
    ["trumpet"]=56 ["trombone"]=57 ["tuba"]=58
    ["muted_trumpet"]=59 ["french_horn"]=60 ["brass_section"]=61
    ["synth_brass"]=62 ["synth_brass2"]=63
    
    # Reed
    ["soprano_sax"]=64 ["alto_sax"]=65 ["tenor_sax"]=66 ["bari_sax"]=67
    ["oboe"]=68 ["english_horn"]=69 ["bassoon"]=70 ["clarinet"]=71
    
    # Pipe
    ["piccolo"]=72 ["flute"]=73 ["recorder"]=74 ["pan_flute"]=75
    ["blown_bottle"]=76 ["shakuhachi"]=77 ["whistle"]=78 ["ocarina"]=79
    
    # Synth Lead
    ["square_lead"]=80 ["sawtooth_lead"]=81 ["calliope_lead"]=82
    ["chiff_lead"]=83 ["charang_lead"]=84 ["voice_lead"]=85
    ["fifths_lead"]=86 ["bass_lead"]=87
    
    # Synth Pad
    ["new_age_pad"]=88 ["warm_pad"]=89 ["polysynth_pad"]=90
    ["choir_pad"]=91 ["bowed_pad"]=92 ["metallic_pad"]=93
    ["halo_pad"]=94 ["sweep_pad"]=95
    
    # Effects
    ["rain"]=96 ["soundtrack"]=97 ["crystal"]=98 ["atmosphere"]=99
    ["brightness"]=100 ["goblins"]=101 ["echoes"]=102 ["sci_fi"]=103
    
    # Ethnic
    ["sitar"]=104 ["banjo"]=105 ["shamisen"]=106 ["koto"]=107
    ["kalimba"]=108 ["bagpipe"]=109 ["fiddle"]=110 ["shanai"]=111
    
    # Percussive
    ["tinkle_bell"]=112 ["agogo"]=113 ["steel_drums"]=114
    ["woodblock"]=115 ["taiko_drum"]=116 ["melodic_tom"]=117
    ["synth_drum"]=118 ["reverse_cymbal"]=119
    
    # Sound Effects
    ["guitar_fret"]=120 ["breath_noise"]=121 ["seashore"]=122
    ["bird_tweet"]=123 ["telephone"]=124 ["helicopter"]=125
    ["applause"]=126 ["gunshot"]=127
)

show_instruments() {
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}                 INSTRUMENTOS DISPONIBLES                 ${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    echo -e "${YELLOW}PIANOS:${NC}"
    echo "  piano, bright_piano, electric_piano, harpsichord, clavinet"
    echo ""
    echo -e "${YELLOW}GUITARRAS:${NC}"
    echo "  nylon_guitar, steel_guitar, jazz_guitar, clean_guitar"
    echo "  overdriven_guitar, distortion_guitar"
    echo ""
    echo -e "${YELLOW}BAJOS:${NC}"
    echo "  acoustic_bass, finger_bass, pick_bass, fretless_bass"
    echo "  slap_bass, synth_bass"
    echo ""
    echo -e "${YELLOW}CUERDAS:${NC}"
    echo "  violin, viola, cello, contrabass, harp, string_ensemble"
    echo ""
    echo -e "${YELLOW}METAL:${NC}"
    echo "  trumpet, trombone, tuba, french_horn, brass_section"
    echo ""
    echo -e "${YELLOW}MADERAS:${NC}"
    echo "  alto_sax, tenor_sax, oboe, clarinet, flute, recorder"
    echo ""
    echo -e "${YELLOW}TECLADOS:${NC}"
    echo "  accordion, harmonica, drawbar_organ, church_organ"
    echo ""
    echo -e "${YELLOW}SINTETIZADOR:${NC}"
    echo "  square_lead, sawtooth_lead, polysynth_pad, warm_pad"
    echo ""
    echo -e "${YELLOW}ÉTNICOS:${NC}"
    echo "  sitar, banjo, shamisen, koto, kalimba, bagpipe"
    echo ""
    echo -e "${YELLOW}PERCUSIÓN:${NC}"
    echo "  steel_drums, timpani, taiko_drum, synth_drum"
    echo ""
}

change_instrument() {
    local inst_name=$1
    local program=${INSTRUMENTS[$inst_name]}
    
    if [[ -z "$program" ]]; then
        echo -e "${RED}Instrumento no encontrado: $inst_name${NC}"
        echo "Use 'list' para ver instrumentos disponibles"
        return 1
    fi
    
    # Send MIDI Program Change (C0 = Program Change Channel 1)
    # Format: C0 pp (where pp is program number 0-127)
    HEX_PC=$(printf "C0 %02X" $program)
    
    # Find FluidSynth port
    FLUID_PORT=$(aconnect -l 2>/dev/null | grep -B1 "FLUID Synth" | grep "client" | grep -oP '\d+')
    
    if [[ -z "$FLUID_PORT" ]]; then
        echo -e "${RED}FluidSynth no encontrado${NC}"
        return 1
    fi
    
    # Send Program Change via amidi (if available) or aconnect
    if command -v amidi &> /dev/null; then
        # Convert port to hw device
        HW_DEV=$(aconnect -l 2>/dev/null | grep -A1 "FLUID Synth" | grep "港口" | grep -oP 'hw:\d+')
        if [[ -n "$HW_DEV" ]]; then
            amidi -p "$HW_DEV" -S "$HEX_PC"
        fi
    fi
    
    # Alternative: use mido via Python
    python3 -c "
import mido
ports = mido.get_output_names()
for port in ports:
    if 'FLUID' in port or 'fluid' in port:
        with mido.open_output(port) as p:
            msg = mido.Message('program_change', program=$program, channel=0)
            p.send(msg)
            print(f'Sent Program Change {msg}')
        break
" 2>/dev/null
    
    echo -e "${GREEN}✓ Instrumento cambiado a: ${YELLOW}$inst_name${NC} (Program #$program)"
}

#══════════════════════════════════════════════════════════════
# MAIN
#══════════════════════════════════════════════════════════════

print_header

if [[ "$1" == "list" ]] || [[ -z "$1" ]]; then
    show_instruments
    echo -e "${YELLOW}Uso: bash change_instrument.sh [nombre_instrumento]${NC}"
    echo -e "${YELLOW}Ejemplo: bash change_instrument.sh flute${NC}"
else
    change_instrument "$1"
fi

echo ""
