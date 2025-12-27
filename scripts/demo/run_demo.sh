#!/bin/bash
set -euo pipefail

echo "1) Run static analyzers"
bash scripts/run_static_analysis.sh || true

echo "2) Build fuzzers (if clang available)"
if command -v clang >/dev/null 2>&1; then
    bash scripts/build_fuzzers.sh || true
fi

echo "3) Run sanitizers and unit tests"
bash scripts/run_sanitizers.sh || true

echo "Demo complete. See scripts/out for artifact logs."