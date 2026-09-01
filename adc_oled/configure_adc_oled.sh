#!/bin/bash
#==============================================================================
# configure_adc_oled.sh - Configurar parámetros del proyecto ADC+OLED por serial
#==============================================================================
# Uso: bash configure_adc_oled.sh
#==============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BAUD_RATE=115200

print_header() {
    echo ""
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║     CONFIGURAR ADC+OLED - RASPBERRY PI PICO             ║${NC}"
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

find_pico_port() {
    local port=""
    for candidate in /dev/ttyACM0 /dev/ttyACM1 /dev/ttyACM2 /dev/ttyUSB0 /dev/ttyUSB1; do
        if [[ -e "$candidate" ]]; then
            port="$candidate"
            break
        fi
    done

    if [[ -z "$port" ]]; then
        print_error "No se encontró ningún puerto serie del Pico"
        echo ""
        print_info "Verifique que el Pico esté conectado por USB"
        print_info "Puertos disponibles:"
        ls -la /dev/ttyACM* 2>/dev/null || true
        ls -la /dev/ttyUSB* 2>/dev/null || true
        exit 1
    fi

    print_step "Puerto encontrado: $port"
    return 0
}

send_command() {
    local port="$1"
    local cmd="$2"
    local timeout="$3"

    # Ensure serial port is configured
    stty -F "$port" 115200 raw -echo 2>/dev/null || true

    # Flush any pending data
    sleep 0.2
    if [[ -r "$port" ]]; then
        timeout 1 cat "$port" > /dev/null 2>/dev/null || true
    fi

    # Small delay before sending
    sleep 0.1

    # Send command with CRLF
    printf "%s\r\n" "$cmd" > "$port" 2>/dev/null || true

    # Wait for response
    local start_time
    start_time=$(date +%s)
    local response=""

    while true; do
        local now
        now=$(date +%s)
        local elapsed=$((now - start_time))

        if [[ $elapsed -ge $timeout ]]; then
            break
        fi

        # Read available data
        if [[ -r "$port" ]]; then
            local chunk
            chunk=$(timeout 0.3 cat "$port" 2>/dev/null || true)
            if [[ -n "$chunk" ]]; then
                # Accumulate all data
                response="${response}${chunk}"

                # Check if we have a complete CFG or VERSION response
                if [[ "$response" =~ \[CFG\] ]] || [[ "$response" =~ \[VERSION\] ]]; then
                    # Wait a bit more for any trailing data
                    sleep 0.3
                    local trailing
                    trailing=$(timeout 0.2 cat "$port" 2>/dev/null || true)
                    if [[ -n "$trailing" ]]; then
                        response="${response}${trailing}"
                    fi
                    break
                fi
            fi
        fi

        sleep 0.05
    done

    # Extract only CFG or VERSION lines
    local filtered=""
    while IFS= read -r line; do
        if [[ "$line" =~ ^\[CFG\] ]] || [[ "$line" =~ ^\[VERSION\] ]]; then
            filtered="${filtered}${line}\n"
        fi
    done <<< "$response"

    # Remove trailing newlines and return
    filtered=$(echo "$filtered" | sed '/^$/d')
    echo "$filtered"
}

configure_trigger() {
    local port="$1"

    echo ""
    print_info "Configurar nivel de trigger (voltaje)"
    echo ""
    echo "El trigger se activa cuando la señal BAJA por debajo de este nivel."
    echo "Ejemplo: 1.700 (rango: 0.000 - 3.300)"
    echo ""

    while true; do
        read -p "Ingrese el voltaje de trigger (3 decimales): " voltage

        # Validate input: must be a float between 0.000 and 3.300
        if [[ ! "$voltage" =~ ^[0-9]+(\.[0-9]{1,3})?$ ]]; then
            print_error "Formato inválido. Use por ejemplo: 1.700, 0.500, 2.100"
            continue
        fi

        # Extract integer and decimal parts
        local int_part="${voltage%%.*}"
        local dec_part="${voltage#*.}"
        dec_part="${dec_part:-0}"
        dec_part=$(printf "%-3s" "$dec_part" | tr ' ' '0')
        dec_part="${dec_part:0:3}"

        local formatted_voltage="${int_part}.${dec_part}"

        # Check range
        local voltage_float
        voltage_float=$(echo "$voltage" | awk '{print $1}')
        if (( $(echo "$voltage_float < 0.0" | bc -l 2>/dev/null || echo 1) )) || \
           (( $(echo "$voltage_float > 3.3" | bc -l 2>/dev/null || echo 0) )); then
            print_error "El voltaje debe estar entre 0.000 y 3.300"
            continue
        fi

        # Send command
        print_info "Enviando: TRIG:$formatted_voltage"
        local response=""
        local attempts=3

        for ((i=1; i<=attempts; i++)); do
            if [[ $i -gt 1 ]]; then
                print_info "Reintentando... ($i/$attempts)"
                sleep 0.3
            fi
            response=$(send_command "$port" "TRIG:$formatted_voltage" 5)
            if [[ -n "$response" ]]; then
                break
            fi
        done

        if [[ -n "$response" ]]; then
            echo ""
            print_step "Respuesta del Pico:"
            echo "$response" | while IFS= read -r line; do
                echo -e "  ${GREEN}>${NC} $line"
            done
        else
            print_warning "No se recibió respuesta del Pico después de $attempts intentos"
        fi

        break
    done
}

configure_scale() {
    local port="$1"

    echo ""
    print_info "Configurar escala/amplitud"
    echo ""
    echo "Valores válidos: 0.1 - 100.0 (con 1 decimal)"
    echo "Ejemplos: 0.5, 1.0, 10.0, 30.0, 50.0, 100.0"
    echo ""

    while true; do
        read -p "Ingrese la escala: " scale

        # Validate input: float between 0.1 and 100.0
        if [[ ! "$scale" =~ ^[0-9]+(\.[0-9])?$ ]]; then
            print_error "Formato inválido. Use por ejemplo: 0.5, 1.0, 10.0, 30.0"
            continue
        fi

        # Check range
        local scale_float
        scale_float=$(echo "$scale" | awk '{print $1}')
        if (( $(echo "$scale_float < 0.1" | bc -l 2>/dev/null || echo 1) )) || \
           (( $(echo "$scale_float > 100.0" | bc -l 2>/dev/null || echo 0) )); then
            print_error "La escala debe estar entre 0.1 y 100.0"
            continue
        fi

        # Normalize to 1 decimal place
        local scale_normalized
        scale_normalized=$(echo "$scale_float" | awk '{printf "%.1f", $1}')

        # Send command
        print_info "Enviando: SCALE:$scale_normalized"
        local response=""
        local attempts=3

        for ((i=1; i<=attempts; i++)); do
            if [[ $i -gt 1 ]]; then
                print_info "Reintentando... ($i/$attempts)"
                sleep 0.3
            fi
            response=$(send_command "$port" "SCALE:$scale_normalized" 5)
            if [[ -n "$response" ]]; then
                break
            fi
        done

        if [[ -n "$response" ]]; then
            echo ""
            print_step "Respuesta del Pico:"
            echo "$response" | while IFS= read -r line; do
                echo -e "  ${GREEN}>${NC} $line"
            done
        else
            print_warning "No se recibió respuesta del Pico después de $attempts intentos"
        fi

        break
    done
}

show_current_config() {
    local port="$1"

    echo ""
    print_info "Solicitando configuración actual"
    echo ""

    local response=""
    local attempts=3

    for ((i=1; i<=attempts; i++)); do
        if [[ $i -gt 1 ]]; then
            print_info "Reintentando... ($i/$attempts)"
            sleep 0.3
        fi
        response=$(send_command "$port" "GET" 5)
        if [[ -n "$response" ]]; then
            break
        fi
    done

    if [[ -n "$response" ]]; then
        print_step "Configuración actual:"
        echo "$response" | while IFS= read -r line; do
            echo -e "  ${GREEN}>${NC} $line"
        done
    else
        print_warning "No se recibió respuesta del Pico después de $attempts intentos"
    fi
}

show_firmware_version() {
    local port="$1"

    echo ""
    print_info "Consultando versión del firmware"
    echo ""

    # Try multiple times in case monitor data interferes
    local response=""
    local attempts=3

    for ((i=1; i<=attempts; i++)); do
        if [[ $i -gt 1 ]]; then
            print_info "Reintentando... ($i/$attempts)"
            sleep 0.5
        fi

        response=$(send_command "$port" "VERSION" 8)

        if [[ -n "$response" ]]; then
            break
        fi
    done

    if [[ -n "$response" ]]; then
        print_step "Versión:"
        echo "$response" | while IFS= read -r line; do
            echo -e "  ${GREEN}>${NC} $line"
        done
    else
        print_warning "No se recibió respuesta del Pico después de $attempts intentos"
        print_info "Verifique que el firmware esté ejecutándose correctamente"
    fi
}

main_menu() {
    local port="$1"

    while true; do
        echo ""
        echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
        echo -e "${BLUE}  MENU DE CONFIGURACION${NC}"
        echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
        echo ""
        echo "  1) Configurar nivel de trigger (voltaje)"
        echo "  2) Configurar escala/amplitud"
        echo "  3) Ver configuración actual"
        echo "  4) Ver versión del firmware"
        echo "  5) Salir"
        echo ""
        read -p "Seleccione una opción: " option

        case "$option" in
            1)
                configure_trigger "$port"
                ;;
            2)
                configure_scale "$port"
                ;;
            3)
                show_current_config "$port"
                ;;
            4)
                show_firmware_version "$port"
                ;;
            5)
                echo ""
                print_step "Saliendo..."
                exit 0
                ;;
            *)
                print_error "Opción inválida"
                ;;
        esac
    done
}

#==============================================================================
# MAIN
#==============================================================================

print_header

# Check dependencies
if ! command -v stty &> /dev/null; then
    print_error "stty no encontrado. Instale util-linux."
    exit 1
fi

if ! command -v cat &> /dev/null; then
    print_error "cat no encontrado."
    exit 1
fi

# Find Pico port
find_pico_port
PORT=$(find_pico_port; echo "$?")

# Actually get the port
PICO_PORT=""
for candidate in /dev/ttyACM0 /dev/ttyACM1 /dev/ttyACM2; do
    if [[ -e "$candidate" ]]; then
        PICO_PORT="$candidate"
        break
    fi
done

if [[ -z "$PICO_PORT" ]]; then
    print_error "No se encontró el puerto del Pico"
    exit 1
fi

print_step "Puerto seleccionado: $PICO_PORT"
echo ""

# Configure serial port
print_info "Configurando puerto serial..."
stty -F "$PICO_PORT" "$BAUD_RATE" raw -echo 2>/dev/null || \
stty "$BAUD_RATE" raw -echo < "$PICO_PORT" 2>/dev/null || true

# Small delay for serial to settle
sleep 0.5

# Flush any pending data
cat "$PICO_PORT" > /dev/null 2>/dev/null &
CAT_PID=$!
sleep 0.2
kill $CAT_PID 2>/dev/null || true

print_step "Puerto serial configurado: $PICO_PORT @ $BAUD_RATE baud"
echo ""

# Show welcome message
echo -e "${GREEN}Conectado al Pico en $PICO_PORT${NC}"
echo ""

# Main menu
main_menu "$PICO_PORT"
