#!/bin/bash
# reproduce_crash.sh <fuzzer> <crashfile> - attempt quick reproduction and save ASAN output
set -euo pipefail
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <path-to-fuzzer> <path-to-crash-file>"
    exit 2
fi
FUZZER=$1
CRASH=$2
OUT_DIR="build/fuzz/artifacts/repro"
mkdir -p "$OUT_DIR"
BASE=$(basename "$CRASH")
LOG="$OUT_DIR/${BASE}.txt"

if [ ! -x "$FUZZER" ]; then
    echo "Fuzzer not executable: $FUZZER" >&2
    exit 1
fi

echo "Running $FUZZER $CRASH" > "$LOG"
# libFuzzer supports single-file execution
"$FUZZER" "$CRASH" >> "$LOG" 2>&1 || true

echo "Reproduction finished; log at $LOG"