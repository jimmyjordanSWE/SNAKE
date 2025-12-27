#!/bin/bash
set -euo pipefail

echo "Building project..."
make

echo "Generating LLM context artifacts (make analyze)..."
make analyze

echo "Running unit tests..."
bash scripts/run_unit_tests.sh

echo "Running fuzz smoke tests..."
bash scripts/run_fuzz_smoke.sh

echo "Project health checks: OK"