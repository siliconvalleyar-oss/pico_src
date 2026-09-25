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
- `project_dir` (optional): Project directory (default: /home/joy/src/pico/pico_src/micro_sd)
- `branch` (optional): Git branch to build (default: microsd_card)
- `compile_only` (optional): Only compile, don't flash (default: false)
- `flash_only` (optional): Only flash, don't compile (default: false)

## IMPORTANT: Work repo
There are TWO clones of pico_src on the Pi:
- `/home/joy/src/pico_src` - OLD clone on branch `main` (do NOT use)
- `/home/joy/src/pico/pico_src` - WORK clone, has sibling SDK at `/home/joy/src/pico/pico-sdk` (USE THIS ONE)

Workflow: edit code locally -> git push -> SSH: git pull -> make -> picotool load.
Or just use the one-step script from the repo: `./scripts/flash_remote.sh`.

## Implementation

### Step 1: Connect to Raspberry Pi
```bash
ssh joy@raspberry.local "echo 'Connected'"
```

### Step 2: Pull latest code (work clone, correct branch)
```bash
ssh joy@raspberry.local "cd /home/joy/src/pico/pico_src && git fetch origin && git checkout microsd_card && git pull origin microsd_card"
```

### Step 3: Compile project (projects have a Makefile wrapper: just make)
```bash
ssh joy@raspberry.local "cd /home/joy/src/pico/pico_src/micro_sd && make -j4 2>&1 | tail -5"
```

### Step 4: Check Pico status
```bash
ssh joy@raspberry.local "lsusb | grep -i 'cafe\|2e8a'"
```

### Step 5: Flash firmware

#### If using picotool:
```bash
# If Pico is in BOOTSEL mode (2e8a:0003)
ssh joy@raspberry.local "picotool load /home/joy/src/pico/pico_src/micro_sd/build/src/micro_sd.uf2 -f"

# If Pico is running (cafe:4004 or firmware PID), reboot to BOOTSEL first
ssh joy@raspberry.local "picotool reboot -u -f && sleep 3 && picotool load /home/joy/src/pico/pico_src/micro_sd/build/src/micro_sd.uf2 -f && picotool reboot"
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
# pico-flash.sh - Complete remote flash script (work clone + branch aware)

HOST="joy@raspberry.local"
REPO="/home/joy/src/pico/pico_src"
PROJECT="micro_sd"          # project folder (has Makefile)
BRANCH="microsd_card"

# Push from the local machine first!
# git push origin $BRANCH

echo "Pulling on Pi..."
ssh $HOST "cd $REPO && git fetch origin && git checkout $BRANCH && git pull origin $BRANCH"

echo "Compiling..."
ssh $HOST "cd $REPO/$PROJECT && make -j4"

echo "Flashing..."
ssh $HOST "picotool reboot -u -f 2>/dev/null; sleep 3; picotool load $REPO/$PROJECT/build/src/$PROJECT.uf2 -f && picotool reboot"

echo "Verifying..."
sleep 3
ssh $HOST "lsusb | grep -i cafe"
```

NOTE: a ready-made script exists in the repo: `scripts/flash_remote.sh`
(pushes local branch, pulls on the Pi, builds with make, flashes with picotool).

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
- Default firmware path: `/home/joy/src/pico/pico_src/micro_sd/build/src/micro_sd.uf2`
- The firmware UF2 output lands in `build/src/` because projects use `add_subdirectory(src)`
- Projects can be built directly with `make` from the project folder (Makefile wrapper around CMake)
- The Pico USB port is used by firmware as MSC (mass storage); stdio goes to UART0 (GP0/GP1)
