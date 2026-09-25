#!/bin/sh
set -e
cd "$(dirname "$0")"
make || true
gcc -Wall -O2 -o test_create_mem test_create_mem.c
./test_create_mem
# run host simulator in background for a short time
./host_simulator /tmp/dhf_mem.bin &
PID=$!
sleep 1
# show dhf_shared struct bytes
hexdump -C /tmp/dhf_mem.bin | sed -n '1,8p'
# stop simulator
kill $PID 2>/dev/null || true
