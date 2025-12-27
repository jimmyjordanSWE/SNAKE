#!/bin/bash
set -euo pipefail

echo "Building with ASAN (debug-asan)..."
make CONFIG=debug-asan build

echo "Running unit tests under ASAN..."
bash scripts/run_unit_tests.sh

echo "Sanitizer smoke run: launching snakegame (short timeout)"
timeout 5s ./snakegame || true

echo "Sanitizer run complete."