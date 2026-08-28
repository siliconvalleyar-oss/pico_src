#!/usr/bin/env python3
"""
start_bt_synth.py - Start FluidSynth with Bluetooth audio output
"""

import subprocess
import time
import sys
import os
import signal

BLUETOOTH_MAC = "AC:EF:92:D0:B5:BB"
SOUNDFONT = "/usr/share/sounds/sf2/FluidR3_GM.sf2"

def run_cmd(cmd):
    """Run a shell command and return output"""
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.stdout.strip(), result.returncode

def main():
    print("Starting FluidSynth with Bluetooth audio...")
    print(f"Bluetooth device: {BLUETOOTH_MAC}")
    
    # Kill any existing FluidSynth
    subprocess.run(["pkill", "-9", "fluidsynth"], capture_output=True)
    time.sleep(1)
    
    # Set Bluetooth as default sink
    print("Setting Bluetooth as default audio sink...")
    run_cmd(f"pactl set-default-sink bluez_sink.{BLUETOOTH_MAC.replace(':', '_')}.a2dp_sink")
    
    # Create a dummy stdin that stays open
    stdin_r, stdin_w = os.pipe()
    
    # Start FluidSynth with PulseAudio
    print("Starting FluidSynth...")
    proc = subprocess.Popen(
        ["fluidsynth", "-a", "pulseaudio", "-g", "1.0", SOUNDFONT],
        stdin=stdin_r,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    # Keep stdin pipe open
    os.close(stdin_w)
    
    time.sleep(3)
    
    # Check if running
    if proc.poll() is not None:
        print("ERROR: FluidSynth failed to start")
        sys.exit(1)
    
    print(f"FluidSynth running (PID: {proc.pid})")
    
    # Move FluidSynth audio to Bluetooth
    print("Routing audio to Bluetooth...")
    output, _ = run_cmd("pactl list sink-inputs short")
    for line in output.split('\n'):
        if str(proc.pid) in line:
            input_id = line.split()[0]
            run_cmd(f"pactl move-sink-input {input_id} bluez_sink.{BLUETOOTH_MAC.replace(':', '_')}.a2dp_sink")
            print(f"Moved FluidSynth (input {input_id}) to Bluetooth")
    
    # Connect MIDI
    print("Connecting MIDI...")
    output, _ = run_cmd("aconnect -l")
    fluid_client = None
    pico_client = None
    
    for line in output.split('\n'):
        if 'FLUID Synth' in line and 'client' in line:
            parts = line.split()
            for part in parts:
                if part.isdigit():
                    fluid_client = part
                    break
        if 'Pico Touch MIDI' in line and 'client' in line:
            parts = line.split()
            for part in parts:
                if part.isdigit():
                    pico_client = part
                    break
    
    if pico_client and fluid_client:
        result = run_cmd(f"aconnect {pico_client}:0 {fluid_client}:0")
        print(f"Connected Pico ({pico_client}) to FluidSynth ({fluid_client})")
    else:
        print("Could not find Pico or FluidSynth MIDI ports")
    
    # Show status
    print("\n" + "="*50)
    print("  BLUETOOTH SYNTH IS READY!")
    print("="*50)
    print(f"  Audio: Xiaomi Sound Pocket ({BLUETOOTH_MAC})")
    print(f"  MIDI: Pico Touch MIDI -> FluidSynth")
    print(f"  SoundFont: General MIDI")
    print("\n  Touch the Pico pads to hear sounds!")
    print("  Press Ctrl+C to stop")
    print("="*50)
    
    # Keep running
    try:
        while True:
            time.sleep(1)
            if proc.poll() is not None:
                print("FluidSynth stopped unexpectedly")
                break
    except KeyboardInterrupt:
        print("\nStopping FluidSynth...")
        proc.terminate()
        proc.wait()
        print("Done")

if __name__ == "__main__":
    main()
