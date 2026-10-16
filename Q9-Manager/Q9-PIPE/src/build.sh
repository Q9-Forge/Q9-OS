#!/bin/bash
set -e

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/.." && pwd)
FORGE="/Volumes/SSD1TB/projects/Q9-Forge"

QR68K="${FORGE}/Q9-SDK/SRC/SYSTEM/Q9-QCC/Q9-BACKEND-68K/q9-qr68k/build/qr68k"
QL68K="${FORGE}/Q9-SDK/SRC/SYSTEM/Q9-QCC/Q9-BACKEND-68K/q9-ql68k/build/ql68k"
DEFS="${ROOT}/defs"

echo "=== Building Q9-PIPE Module (pipeman.mod) ==="
cd "$HERE"

# Ensure defs are accessible locally
if [ ! -f "q9defs.d" ]; then
    cp "$DEFS/q9defs.d" "q9defs.d"
fi
if [ ! -f "pipe_defs.a" ]; then
    cp "$DEFS/pipe_defs.a" "pipe_defs.a"
fi

echo "Assembling pipe_68k.a -> pipe_68k.r..."
"$QR68K" pipe_68k.a -o=pipe_68k.r

echo "Linking pipe_68k.r -> pipeman.mod..."
"$QL68K" pipe_68k.r -n=pipeman -gu=0.0 -O=pipeman.mod

python3 "$ROOT/tools/fixmod.py" pipeman.mod

echo "Build successful: $(pwd)/pipeman.mod"
ls -la pipeman.mod
