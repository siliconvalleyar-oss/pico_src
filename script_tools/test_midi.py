#!/usr/bin/env python3
"""
test_midi.py - Receber e exibir mensagens MIDI do Raspberry Pi Pico
====================================================================
Uso: python3 test_midi.py
====================================================================
Dependências: pip install mido python-rtmidi
"""

import sys
import time
from datetime import datetime

try:
    import mido
except ImportError:
    print("Error: mido no está instalado")
    print("Ejecute: pip install mido python-rtmidi")
    sys.exit(1)

# Colores ANSI
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    MAGENTA = '\033[0;35m'
    CYAN = '\033[0;36m'
    WHITE = '\033[1;37m'
    RESET = '\033[0m'

# Notas MIDI
NOTES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

def midi_note_to_name(note):
    """Convertir número de nota MIDI a nombre"""
    octave = (note // 12) - 1
    note_name = NOTES[note % 12]
    return f"{note_name}{octave}"

def print_header():
    """Imprimir encabezado"""
    print(f"""
{Colors.CYAN}╔══════════════════════════════════════════════════════════╗
║          RECEPTOR MIDI - RASPBERRY PI PICO             ║
╚══════════════════════════════════════════════════════════╝{Colors.RESET}

{Colors.YELLOW}Notas del Pico:{Colors.RESET}
  Pad 1 (GPIO 26): C4 (Do)
  Pad 2 (GPIO 27): D4 (Re)
  Pad 3 (GPIO 28): E4 (Mi)

{Colors.YELLOW}Presione Ctrl+C para salir{Colors.RESET}
{Colors.CYAN}{'='*58}{Colors.RESET}
""")

def list_midi_ports():
    """Listar puertos MIDI disponibles"""
    print(f"{Colors.BLUE}Puertos MIDI de entrada:{Colors.RESET}")
    input_ports = mido.get_input_names()
    
    if not input_ports:
        print(f"{Colors.RED}  No se encontraron puertos MIDI{Colors.RESET}")
        return None
    
    for i, port in enumerate(input_ports):
        marker = " ◄── Pico" if "Pico" in port or "cafe" in port.lower() else ""
        print(f"  [{i+1}] {port}{Colors.GREEN}{marker}{Colors.RESET}")
    
    return input_ports

def find_pico_port(input_ports):
    """Buscar el puerto MIDI del Pico"""
    for port in input_ports:
        if "Pico" in port or "pico" in port.lower() or "cafe" in port.lower():
            return port
    return None

def process_midi_message(msg):
    """Procesar y mostrar mensaje MIDI"""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    
    if msg.type == 'note_on':
        note_name = midi_note_to_name(msg.note)
        velocity = msg.velocity
        
        if velocity > 0:
            # Note On
            bar_len = int(velocity / 127 * 20)
            bar = "█" * bar_len + "░" * (20 - bar_len)
            
            print(f"{timestamp} {Colors.GREEN}● NOTE ON {Colors.RESET} "
                  f"Canal:{Colors.CYAN}{msg.channel+1}{Colors.RESET} "
                  f"Nota:{Colors.YELLOW}{note_name:3s}{Colors.RESET} "
                  f"({msg.note:3d}) "
                  f"Velocity:{Colors.MAGENTA}{velocity:3d}{Colors.RESET} "
                  f"│{Colors.GREEN}{bar}{Colors.RESET}│")
        else:
            # Note Off (velocity 0)
            print(f"{timestamp} {Colors.RED}○ NOTE OFF{Colors.RESET} "
                  f"Canal:{Colors.CYAN}{msg.channel+1}{Colors.RESET} "
                  f"Nota:{Colors.YELLOW}{note_name:3s}{Colors.RESET} "
                  f"({msg.note:3d})")
    
    elif msg.type == 'note_off':
        note_name = midi_note_to_name(msg.note)
        print(f"{timestamp} {Colors.RED}○ NOTE OFF{Colors.RESET} "
              f"Canal:{Colors.CYAN}{msg.channel+1}{Colors.RESET} "
              f"Nota:{Colors.YELLOW}{note_name:3s}{Colors.RESET} "
              f"({msg.note:3d})")
    
    elif msg.type == 'control_change':
        print(f"{timestamp} {Colors.BLUE}◆ CC{Colors.RESET}      "
              f"Canal:{Colors.CYAN}{msg.channel+1}{Colors.RESET} "
              f"CC#{msg.control:3d} "
              f"Value:{Colors.MAGENTA}{msg.value:3d}{Colors.RESET} "
              f"│{'█' * int(msg.value/127*20):20s}│")
    
    elif msg.type == 'pitchwheel':
        value = msg.pitch
        print(f"{timestamp} {Colors.YELLOW}◇ PITCH{Colors.RESET}   "
              f"Canal:{Colors.CYAN}{msg.channel+1}{Colors.RESET} "
              f"Value:{Colors.MAGENTA}{value:6d}{Colors.RESET}")
    
    elif msg.type == 'aftertouch':
        print(f"{timestamp} {Colors.MAGENTA}◈ AFTERTOUCH{Colors.RESET} "
              f"Canal:{Colors.CYAN}{msg.channel+1}{Colors.RESET} "
              f"Pressure:{Colors.MAGENTA}{msg.pressure:3d}{Colors.RESET}")
    
    else:
        print(f"{timestamp} {Colors.WHITE}? {msg.type}{Colors.RESET}: {msg}")

def main():
    """Función principal"""
    print_header()
    
    # Listar puertos
    input_ports = list_midi_ports()
    
    if not input_ports:
        print(f"\n{Colors.RED}No se encontraron puertos MIDI.{Colors.RESET}")
        print(f"Verifique que el Pico esté conectado con:")
        print(f"  {Colors.YELLOW}lsusb | grep cafe{Colors.RESET}")
        sys.exit(1)
    
    # Buscar Pico
    pico_port = find_pico_port(input_ports)
    
    if pico_port:
        print(f"\n{Colors.GREEN}✓ Pico encontrado: {pico_port}{Colors.RESET}")
        selected_port = pico_port
    else:
        print(f"\n{Colors.YELLOW}Pico no encontrado automáticamente.{Colors.RESET}")
        try:
            choice = int(input("Seleccione puerto [1]: ") or "1") - 1
            selected_port = input_ports[choice]
        except (ValueError, IndexError):
            selected_port = input_ports[0]
    
    # Conectar al puerto
    print(f"\n{Colors.BLUE}Conectando a: {selected_port}{Colors.RESET}")
    
    try:
        with mido.open_input(selected_port) as port:
            print(f"{Colors.GREEN}✓ Conectado! Esperando mensajes MIDI...{Colors.RESET}\n")
            print(f"{Colors.CYAN}{'='*58}{Colors.RESET}")
            
            msg_count = 0
            
            while True:
                try:
                msg = port.receive(timeout=1.0)
            except TypeError:
                # Fallback for older mido versions
                msg = port.receive()
                
                if msg is not None:
                    msg_count += 1
                    process_midi_message(msg)
                    
    except KeyboardInterrupt:
        print(f"\n\n{Colors.CYAN}{'='*58}{Colors.RESET}")
        print(f"{Colors.YELLOW}Sesión terminada{Colors.RESET}")
        print(f"Mensajes recibidos: {Colors.GREEN}{msg_count}{Colors.RESET}")
        print()
    except Exception as e:
        print(f"\n{Colors.RED}Error: {e}{Colors.RESET}")
        sys.exit(1)

if __name__ == "__main__":
    main()
