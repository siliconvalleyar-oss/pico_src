# Skill: pico-remote-flash

## Description
Compile and flash firmware to a Raspberry Pi Pico connected to a remote Raspberry Pi via SSH.

## Trigger Phrases
- "flash pico"
- "load firmware"
- "compile and flash"
- "update pico"
- "subir firmware al pico"
- "flashear pico"
- "cargar firmware"

## Parameters
- `host` (optional): SSH host address (default: joy@raspberry.local)
- `project_dir` (optional): Project directory (default: /home/joy/src/pico_src/midi)
- `compile_only` (optional): Only compile, don't flash (default: false)
- `flash_only` (optional): Only flash, don't compile (default: false)

## Implementation

### Step 1: Connect to Raspberry Pi
```bash
ssh joy@raspberry.local "echo 'Connected'"
```

### Step 2: Pull latest code
```bash
ssh joy@raspberry.local "cd /home/joy/src/pico_src && git pull"
```

### Step 3: Compile project
```bash
ssh joy@raspberry.local "cd /home/joy/src/pico_src/midi && \
    rm -rf build && mkdir build && cd build && \
    cmake .. 2>&1 | tail -3 && \
    make -j4 2>&1 | tail -5"
```

### Step 4: Check Pico status
```bash
ssh joy@raspberry.local "lsusb | grep -i 'cafe\|2e8a'"
```

### Step 5: Flash firmware

#### If using picotool:
```bash
# If Pico is in BOOTSEL mode (2e8a:0003)
ssh joy@raspberry.local "picotool load /home/joy/src/pico_src/midi/build/src/midi.uf2 -f"

# If Pico is running (cafe:4015), reboot to BOOTSEL first
ssh joy@raspberry.local "picotool reboot -u && sleep 3 && picotool load /home/joy/src/pico_src/midi/build/src/midi.uf2 -f"
```

#### If using manual copy:
```bash
ssh joy@raspberry.local "sudo mkdir -p /mnt/pico && \
    sudo mount /dev/sda1 /mnt/pico && \
    sudo cp /home/joy/src/pico_src/midi/build/src/midi.uf2 /mnt/pico/ && \
    sync && sudo umount /mnt/pico"
```

### Step 6: Verify
```bash
sleep 3 && ssh joy@raspberry.local "lsusb | grep cafe"
```

## Complete Script
```bash
#!/bin/bash
# pico-flash.sh - Complete remote flash script

HOST="joy@raspberry.local"
PROJECT="/home/joy/src/pico_src/midi"

echo "Connecting to Raspberry Pi..."
ssh $HOST "cd /home/joy/src/pico_src && git pull"

echo "Compiling..."
ssh $HOST "cd $PROJECT && rm -rf build && mkdir build && cd build && cmake .. && make -j4"

echo "Flashing..."
ssh $HOST "picotool reboot -u 2>/dev/null; sleep 3; picotool load $PROJECT/build/src/midi.uf2 -f 2>/dev/null || (sudo mount /dev/sda1 /mnt/pico && sudo cp $PROJECT/build/src/midi.uf2 /mnt/pico/ && sync && sudo umount /mnt/pico)"

echo "Verifying..."
sleep 3
ssh $HOST "lsusb | grep cafe"
```

## Error Handling

### Pico not detected
```bash
echo "Pico not detected. Check USB connection."
echo "Run: lsusb | grep -i 'cafe\|2e8a'"
```

### Permission denied
```bash
# Use sudo for mount operations
sudo mount /dev/sda1 /mnt/pico
```

### Compilation failed
```bash
# Check dependencies
ssh joy@raspberry.local "arm-none-eabi-gcc --version"
ssh joy@raspberry.local "cmake --version"
```

## Notes
- The Pico must be in BOOTSEL mode for manual copy method
- picotool can force reboot to BOOTSEL mode
- Always verify with `lsusb | grep cafe` after flashing
- Default firmware path: `/home/joy/src/pico_src/midi/build/src/midi.uf2`
