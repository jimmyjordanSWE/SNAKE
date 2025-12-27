#!/bin/bash
# Run static analyzers (cppcheck, clang-tidy) and record outputs
set -euo pipefail
OUT_DIR="scripts/out"
mkdir -p "$OUT_DIR"

echo "Running cppcheck..."
if command -v cppcheck >/dev/null 2>&1; then
    cppcheck --enable=all --inconclusive --template=gcc -Iinclude -Isrc 2> "$OUT_DIR/cppcheck.txt" || true
else
    echo "cppcheck not installed; skipping" > "$OUT_DIR/cppcheck.txt"
fi

echo "Running clang-tidy..."
if command -v clang-tidy >/dev/null 2>&1; then
    # Collect list of C files
    for f in $(git ls-files '*.c'); do
        echo "clang-tidy on $f" >> "$OUT_DIR/clang-tidy.txt"
        clang-tidy "$f" -- -Iinclude -Isrc -std=c99 >> "$OUT_DIR/clang-tidy.txt" 2>&1 || true
    done
else
    echo "clang-tidy not installed; skipping" > "$OUT_DIR/clang-tidy.txt"
fi

echo "Static analysis complete. Outputs in $OUT_DIR"