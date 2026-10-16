#!/bin/bash
set -e

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/.." && pwd)
FORGE="/Volumes/SSD1TB/projects/Q9-Forge"

QR68K="${FORGE}/Q9-SDK/SRC/SYSTEM/Q9-QCC/Q9-BACKEND-68K/q9-qr68k/build/qr68k"
QL68K="${FORGE}/Q9-SDK/SRC/SYSTEM/Q9-QCC/Q9-BACKEND-68K/q9-ql68k/build/ql68k"
DEFS="${ROOT}/defs/q9defs.d"

echo "=== Building RBF68k Module ==="
cd "$HERE"

# Copy defs into local dir if not present
if [ ! -f "q9defs.d" ]; then
    cp "$DEFS" "q9defs.d"
fi

echo "Assembling rbf_68k.a -> rbf_68k.r..."
"$QR68K" rbf_68k.a -o=rbf_68k.r

echo "Linking rbf_68k.r -> rbf.mod..."
"$QL68K" rbf_68k.r -n=rbf -gu=0.0 -O=rbf.mod

python3 "$ROOT/tools/fixmod.py" rbf.mod

echo "Build successful: $(pwd)/rbf.mod"
ls -la rbf.mod
