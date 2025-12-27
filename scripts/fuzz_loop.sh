#!/bin/bash
# fuzz_loop.sh - run fuzzers continuously in background and keep logs
set -euo pipefail
OUT_DIR="build/fuzz/artifacts"
LOG_DIR="$OUT_DIR/logs"
mkdir -p "$OUT_DIR" "$LOG_DIR"

# Build fuzzers first
bash scripts/build_fuzzers.sh || exit 1

# Start each fuzzer in its own background process with nohup
# The artifact_prefix tells libFuzzer where to write crashes/hangs
nohup build/fuzz/fuzz_net -artifact_prefix="$OUT_DIR/net_" > "$LOG_DIR/net.out" 2>&1 &
NET_PID=$!
nohup build/fuzz/fuzz_persist -artifact_prefix="$OUT_DIR/persist_" > "$LOG_DIR/persist.out" 2>&1 &
PERSIST_PID=$!

echo "Started fuzz_net (pid=$NET_PID) and fuzz_persist (pid=$PERSIST_PID). Logs: $LOG_DIR"

echo "PIDs: $NET_PID $PERSIST_PID" > "$OUT_DIR/pids.txt"