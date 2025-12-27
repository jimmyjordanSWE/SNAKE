#!/bin/bash
set -euo pipefail
OUT_DIR="build/fuzz/artifacts"
LOG_DIR="$OUT_DIR/logs"
mkdir -p "$OUT_DIR" "$LOG_DIR"

# Build fuzzers first
bash scripts/build_fuzzers.sh

# Ensure corpus directory exists
CORPUS="build/fuzz/corpus/stb_image"
mkdir -p "$CORPUS"

# Default extended time is 3 hours (10800s) if not provided
TIME=${1:-10800}

echo "Running extended fuzz for fuzz_stb_image for ${TIME}s..."
build/fuzz/fuzz_stb_image -max_total_time=$TIME -artifact_prefix="$OUT_DIR/stb_image_" -rss_limit_mb=1024 "$CORPUS" > "$LOG_DIR/fuzz_stb_image.extended.out" 2>&1 || {
    echo "Extended fuzzer failed. See $LOG_DIR/fuzz_stb_image.extended.out"
    exit 1
}

echo "Extended fuzz complete. Artifacts in $OUT_DIR"