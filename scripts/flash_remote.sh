#!/bin/bash
# flash_remote.sh - Compila y flashea la Pico en la Raspberry Pi remota en un
# solo paso.
#
#   local: git push -> remota: git pull -> make -j4 -> picotool load -f
#
# Uso:
#   ./scripts/flash_remote.sh                 # micro_sd (default)
#   ./scripts/flash_remote.sh micro_sd        # proyecto explicito
#   ./scripts/flash_remote.sh --no-push       # no hacer push local antes
#
# Variables (env):
#   REMOTE_HOST=joy@raspberry.local
#   REMOTE_REPO=/home/joy/src/pico/pico_src
#   BRANCH=microsd_card          (default: rama actual local)
#   SKIP_BUILD=0                 (1: no recompilar en la remota)

set -euo pipefail

REMOTE_HOST="${REMOTE_HOST:-joy@raspberry.local}"
REMOTE_REPO="${REMOTE_REPO:-/home/joy/src/pico/pico_src}"
PROJECT="${1:-micro_sd}"
DO_PUSH=1

for arg in "$@"; do
    if [ "$arg" = "--no-push" ]; then DO_PUSH=0; fi
done

# Rama local actual (si git esta disponible)
BRANCH="${BRANCH:-$(git branch --show-current 2>/dev/null || echo microsd_card)}"

echo "=== Flash remoto de '${PROJECT}' ==="
echo "host   : ${REMOTE_HOST}"
echo "repo   : ${REMOTE_REPO}"
echo "rama   : ${BRANCH}"
echo ""

# ---------------------------------------------------------------------------
# Paso 1: push local (o fetch en remota si --no-push)
# ---------------------------------------------------------------------------
if [ "${DO_PUSH}" = "1" ]; then
    echo "--- Paso 1/4: git push origin ${BRANCH} ---"
    git push origin "${BRANCH}"
else
    echo "--- Paso 1/4: push omitido (--no-push) ---"
fi
echo ""

# ---------------------------------------------------------------------------
# Paso 2: pull en la remota
# ---------------------------------------------------------------------------
echo "--- Paso 2/4: git pull en ${REMOTE_HOST} ---"
ssh "${REMOTE_HOST}" "cd '${REMOTE_REPO}' && git fetch origin && git checkout '${BRANCH}' && git pull origin '${BRANCH}'"
echo ""

# ---------------------------------------------------------------------------
# Paso 3: compilar (make -j4 dentro del proyecto)
# ---------------------------------------------------------------------------
if [ "${SKIP_BUILD:-0}" != "1" ]; then
    echo "--- Paso 3/4: make -j4 (${PROJECT}) ---"
    ssh "${REMOTE_HOST}" "cd '${REMOTE_REPO}/${PROJECT}' && make -j4"
else
    echo "--- Paso 3/4: compilacion omitida (SKIP_BUILD=1) ---"
fi
echo ""

# ---------------------------------------------------------------------------
# Paso 4: flashear
# ---------------------------------------------------------------------------
UF2_REMOTE="${REMOTE_REPO}/${PROJECT}/build/src/${PROJECT}.uf2"

echo "--- Paso 4/4: flashear ${UF2_REMOTE} ---"

# Si picotool ve la Pico corriendo (PID cafe:4004), reiniciarla a BOOTSEL
ssh "${REMOTE_HOST}" "
    if picotool info 2>/dev/null | grep -q 'Partition'; then
        echo 'Pico detectada: reiniciando a BOOTSEL...'
        picotool reboot -u -f 2>/dev/null || true
        sleep 3
    fi
    picotool load '${UF2_REMOTE}' -f && picotool reboot
"

echo ""
echo "=== Listo: firmware '${PROJECT}' flasheado ==="
