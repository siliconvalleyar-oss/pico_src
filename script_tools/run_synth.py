#!/usr/bin/env python3
"""
run_synth.py - Run FluidSynth and connect to Pico MIDI
"""

import subprocess
import time
import sys
import os
import signal

def main():
    print("Starting FluidSynth...")
    
    # Kill any existing FluidSynth
    subprocess.run(["pkill", "fluidsynth"], capture_output=True)
    time.sleep(1)
    
    # Start FluidSynth
    proc = subprocess.Popen(
        ["fluidsynth", "-a", "alsa", "-m", "alsa_seq", 
         "/usr/share/sounds/sf2/FluidR3_GM.sf2"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    print(f"FluidSynth started with PID: {proc.pid}")
    time.sleep(3)
    
    # Check if it's running
    if proc.poll() is None:
        print("✓ FluidSynth is running")
    else:
        print("✗ FluidSynth failed to start")
        sys.exit(1)
    
    # List ALSA clients
    result = subprocess.run(["aconnect", "-l"], capture_output=True, text=True)
    print("\nALSA MIDI clients:")
    print(result.stdout)
    
    # Find Pico and FluidSynth clients
    pico_client = None
    fluid_client = None
    
    for line in result.stdout.split('\n'):
        if 'Pico Touch MIDI' in line and 'client' in line:
            parts = line.split()
            for part in parts:
                if part.isdigit():
                    pico_client = part
                    break
        if 'FLUID Synth' in line and 'client' in line:
            parts = line.split()
            for part in parts:
                if part.isdigit():
                    fluid_client = part
                    break
    
    if pico_client and fluid_client:
        print(f"Connecting Pico ({pico_client}) to FluidSynth ({fluid_client})...")
        conn = subprocess.run(
            ["aconnect", pico_client + ":0", fluid_client + ":0"],
            capture_output=True, text=True
        )
        if conn.returncode == 0:
            print("✓ Connected!")
        else:
            print(f"Connection error: {conn.stderr}")
    else:
        print("Could not find Pico or FluidSynth clients")
        print("Pico client:", pico_client)
        print("Fluid client:", fluid_client)
    
    print("\n" + "="*50)
    print("  SYNTH IS READY!")
    print("  Touch the Pico pads to hear sounds!")
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
