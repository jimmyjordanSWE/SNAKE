#!/bin/bash
# monitor_artifacts.sh - watch fuzz artifacts and reproduce crashes for triage
set -euo pipefail
ART=%r/build/fuzz/artifacts
ART_DIR="build/fuzz/artifacts"
CRASH_DIR="$ART_DIR/crashes"
LOG_DIR="$ART_DIR/triage_logs"
mkdir -p "$CRASH_DIR" "$LOG_DIR"

# find all artifact files (non-empty) and try to reproduce them with the matching fuzzer
# This basic script can be run from cron or a loop in a separate session

for f in "$ART_DIR"/*; do
    # skip directories and logs
    [ -f "$f" ] || continue
    base=$(basename "$f")
    # skip files produced by our log rotation
    case "$base" in
        *_out|*_log|pids.txt) continue ;;
    esac

    # If we already triaged this file, skip
    if [ -f "$LOG_DIR/${base}.triaged" ]; then
        continue
    fi

    # Decide fuzzer by prefix
    if [[ "$base" == net_* || "$base" == net-* ]]; then
        FUZZER="build/fuzz/fuzz_net"
    elif [[ "$base" == persist_* || "$base" == persist-* ]]; then
        FUZZER="build/fuzz/fuzz_persist"
    else
        # unknown artifact, copy to crashes and mark
        cp -v "$f" "$CRASH_DIR/"
        touch "$LOG_DIR/${base}.triaged"
        continue
    fi

    if [ ! -x "$FUZZER" ]; then
        echo "Fuzzer $FUZZER not found; skipping repro for $base" > "$LOG_DIR/${base}.triaged"
        continue
    fi

    echo "Reproducing $base with $FUZZER..." > "$LOG_DIR/${base}.repro.log"
    # Run fuzzer with the single test case as input; capture STDERR which contains ASAN output
    "$FUZZER" "$f" > "$LOG_DIR/${base}.repro.out" 2> "$LOG_DIR/${base}.repro.err" || true
    cp -v "$f" "$CRASH_DIR/"
    touch "$LOG_DIR/${base}.triaged"
done

echo "Monitoring run complete. Crashes copied to $CRASH_DIR; triage logs in $LOG_DIR."