#!/bin/bash
set -euo pipefail
OUT_DIR="build/fuzz/artifacts"
LOG_DIR="$OUT_DIR/logs"
mkdir -p "$OUT_DIR" "$LOG_DIR"

# Build fuzzers first
bash scripts/build_fuzzers.sh

# Run each fuzzer for a deterministic, bounded number of runs to smoke-test for obvious crashes.
for f in build/fuzz/fuzz_*; do
    [ -x "$f" ] || continue
    name=$(basename "$f")
    echo "Running smoke fuzz for $name..."
    # -runs=1000 ensures the fuzzer finishes quickly; adjust as needed for CI time limits.
    "$f" -artifact_prefix="$OUT_DIR/${name}_" -runs=1000 > "$LOG_DIR/${name}.out" 2>&1 || {
        echo "Fuzzer $name failed. See $LOG_DIR/${name}.out"
        exit 1
    }
    echo "Done: $name (logs: $LOG_DIR/${name}.out)"
done

echo "Fuzz smoke tests completed. Artifacts: $OUT_DIR"